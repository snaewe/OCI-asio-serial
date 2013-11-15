#include "GPSSentenceDecoder.h"
#include "Util.h"
#include <cctype>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp> 
#include <boost/regex.hpp>

void GPSLib::GPSSentenceDecoder::AddBytes(boost::asio::io_service &ios, const std::vector<unsigned char> &bufferToAdd, size_t bufferSize) {
	boost::mutex::scoped_lock lock(_bufferMutex);

	if (_decodeStrand == 0)
		_decodeStrand = boost::shared_ptr<boost::asio::strand>(new boost::asio::strand(ios));

	// pass bufferSize in case buffer has size greater than the amount of meaningful data in it
	std::for_each(bufferToAdd.begin(), (bufferSize == -1) ? bufferToAdd.end() : (bufferToAdd.begin() + bufferSize), [this](const unsigned char &c) {
		if (std::isprint(c) || (c=='\r') || (c == '\n'))  // a sentence is ASCII plus CR-LF - ignore anything out of that range
			_buffer += c;
	});

	size_t pos;
	while ( (pos = _buffer.find("\r\n")) != std::string::npos) {  // tokenize on \r\n which ends a string
		const std::string s(_buffer.substr(0, pos+2));  // +2 for \r\n
		_buffer.erase(0, pos+2);
		// post this to io_service through a strand to keep order of decode the same as order of arrival (as some messages may decode faster than others)
		_decodeStrand->post(boost::bind(&GPSSentenceDecoder::Decode, shared_from_this(), boost::ref(ios), s));  
	}
}


namespace {
	bool TokenizeSentence(const std::string &s, std::vector<std::string> &tokens) {
		tokens.clear();
		const std::string::size_type dollarPos = s.find("$");
		const std::string::size_type CRLFPos = s.find("\r\n");

		// fail if can't find sentence boundary, or sentence too short
		if ((dollarPos == std::string::npos) ||
			(CRLFPos == std::string::npos) ||
			(CRLFPos - dollarPos < 5)) {
				return false;
		}

		std::string::size_type textEndPos = CRLFPos;
		const std::string::size_type starPos = s.rfind("*");
		if (starPos != std::string::npos) {
			// have a checksum, so validate it
			const char expected_checksum_bytes[] = { s[starPos+1], s[starPos+2], 0 };  // checksum is just prior to the CRLF
			const unsigned char expected_checksum = boost::lexical_cast<GPSLib::byte_from_hex>(expected_checksum_bytes);

			unsigned char calculated_checksum = 0;
			std::for_each(s.begin()+dollarPos+1, s.begin()+starPos, [&calculated_checksum](char c) { calculated_checksum^=c; });  // +1, since only chars between $ *

			// fail if checksum mismatch
			if (calculated_checksum != expected_checksum)
				return false;

			textEndPos = starPos;
		}

		// only tokenize from char following $ to before *, if it exists
		const boost::char_separator<char> sep(",", 0, boost::keep_empty_tokens);
		const boost::tokenizer<boost::char_separator<char>> t(s.begin()+dollarPos, s.begin()+textEndPos, sep);
		std::copy(t.begin(), t.end(), std::back_inserter(tokens));
		return tokens.size()>0;  // parse is good if there is at least one token
	}

	const boost::regex hms("(\\d{2})(\\d{2})(\\d{2})(?:(.\\d*))?");
	boost::posix_time::time_duration DecodeTime(std::vector<std::string>::iterator &i) {
		boost::smatch m;
		if (boost::regex_match(*i, m, hms)) {
			i++;  // consume the match
			const boost::posix_time::hours hr(boost::lexical_cast<int>(m[1].str()));
			const boost::posix_time::minutes min(boost::lexical_cast<int>(m[2].str()));
			const boost::posix_time::seconds sec(boost::lexical_cast<int>(m[3].str()));
			const boost::posix_time::milliseconds ms(boost::lexical_cast<int>(boost::lexical_cast<double>(m[4].str())*1000));
			return boost::posix_time::time_duration(hr+min+sec+ms);
		}
		else
			throw std::invalid_argument(*i + " is not hms");
	}

	const boost::regex latlng("(\\d{2,3})(\\d{2}.\\d+)");
	double DecodeLatLng(std::vector<std::string>::iterator &i) {
		boost::smatch m;
		if (boost::regex_match(*i, m, latlng)) {
			i++; // consume the match
			double deg = boost::lexical_cast<double>(m[1].str());
			double min = boost::lexical_cast<double>(m[2].str());
			std::string hemisphere = *i++;
			return GPSLib::ToDecimalDegree(deg, min, hemisphere);
		}
		else
			throw std::invalid_argument(*i + " is not lat or lng");
	}

	const boost::regex dmy("(\\d{2})(\\d{2})(\\d{2})");
	boost::gregorian::date DecodeDate(std::vector<std::string>::iterator &i) {
		boost::smatch m;
		if (boost::regex_match(*i, m, dmy)) {
			i++; // consume the match
			int day = boost::lexical_cast<int>(m[1].str());
			int month = boost::lexical_cast<int>(m[2].str());
			int year = boost::lexical_cast<int>(m[3].str()) + 2000;
			return boost::gregorian::date(year, month, day);
		}
		else
			throw std::invalid_argument(*i + " is not dmy");
	}
}

void GPSLib::GPSSentenceDecoder::Decode(boost::asio::io_service &ios, const std::string &s) {
	try {
		std::vector<std::string> v;
		if (!TokenizeSentence(s, v)) {
			if (OnInvalidSentence) 
				OnInvalidSentence(ios, s);
			return;
		}

		std::vector<std::string>::iterator i = v.begin();
		if (*i == "$GPGGA") {
			// GGA = Global Positioning System Fix Data
			// $GPGGA,191630.609,3848.2905,N,09018.4239,W,1,06,1.3,132.0,M,-33.7,M,0.0,0000*48
			i++;  // consume the $GPGGA token
			const boost::posix_time::time_duration time(DecodeTime(i));
			const double lat = DecodeLatLng(i);
			const double lng = DecodeLatLng(i);
			const int quality = lexical_cast_default<int>(*i++, 0);
			const int numSatellites = lexical_cast_default<int>(*i++, 0);
			const double horizontalDilution = lexical_cast_default<double>(*i++, 0);
			const double altitude = lexical_cast_default<double>(*i++, 0);

			if (OnGGA)
				OnGGA(ios, time, lat, lng, quality, numSatellites, horizontalDilution, altitude);
			return;
		}


		if (*i == "$GPGLL") {
			// GLL = Geographic Position, Latitude / Longitude and time
			// $GPGLL,3848.2905,N,09018.4239,W,191630.609,A*20
			i++;  // consume the $GPGLL token
			double lat = DecodeLatLng(i);
			double lng = DecodeLatLng(i);
			boost::posix_time::time_duration time(DecodeTime(i));
			std::string validity(*i++);

			if (OnGLL)
				OnGLL(ios, time, lat, lng, validity);
			return;
		}


		if (*i == "$GPRMC") {
			// RMC = Recommended minimum specific GPS/Transit data 
			// $GPRMC,191632.609,A,3848.3005,N,09018.4051,W,32.523475,55.89,150113,,*14
			i++;  // consume the $GPRMC token
			boost::posix_time::time_duration time(DecodeTime(i));
			std::string validity(*i++);
			double lat = DecodeLatLng(i);
			double lng = DecodeLatLng(i);
			double speed = lexical_cast_default<double>(*i++, 0);  // knots
			double course = lexical_cast_default<double>(*i++, 0);
			boost::gregorian::date date(DecodeDate(i));

			if (OnRMC)
				OnRMC(ios, time, lat, lng, speed, course, date, validity);
			return;
		}


		if (*i == "$GPGSV") {
			// GSV = GPS Satellites in view
			// $GPGSV,3,1,10,18,62,311,37,15,47,49,40,14,16,218,30,29,11,186,28*4A
			i++;  // consume the $GPGSV token
			int totalMessages = lexical_cast_default<int>(*i++, 0);
			int messageNumber = lexical_cast_default<int>(*i++, 0);
			int totalSatellitesInView = lexical_cast_default<int>(*i++, 0);
			std::vector<SatelliteInfo> satelliteInfo;
			while (i!=v.end()) {
				int prn = lexical_cast_default<int>(*i++, 0);
				int elevation = lexical_cast_default<int>(*i++, 0);
				int azimuth = lexical_cast_default<int>(*i++, 0);
				int snr = lexical_cast_default<int>(*i++, 0);
				satelliteInfo.push_back(SatelliteInfo(prn, elevation, azimuth, snr));
			}

			if (OnGSV)
				OnGSV(ios, totalMessages, messageNumber, totalSatellitesInView, satelliteInfo);
			return;
		}


		if (*i == "$GPGSA") {
			// GSA = GPS DOP and active satellites
			// $GPGSA,A,3,18,15,21,06,09,,,,,,,,3.7,2.8,2.3*3C
			i++;  // consume the $GPGSA token
			std::string mode(*i++);
			int fix = lexical_cast_default<int>(*i++, 1);  // 1=fix not available
			std::vector<int> satellitesInView;
			while (i<v.begin()+15) {
				int sv = lexical_cast_default<int>(*i++, -1);
				if (sv != -1)
					satellitesInView.push_back(sv);
			}
			double pdop = lexical_cast_default<double>(*i++, 0);
			double hdop = lexical_cast_default<double>(*i++, 0);
			double vdop = lexical_cast_default<double>(*i++, 0);

			if (OnGSA)
				OnGSA(ios, mode, fix, satellitesInView, pdop, hdop, vdop);
			return;
		}

		// didn't parse anything
		if (OnInvalidSentence) 
			OnInvalidSentence(ios, s);

	} catch (const std::exception &e) {
		std::cout << "Error while decoding >" << s << "< : " << e.what() << std::endl;
		throw;
	}
}
