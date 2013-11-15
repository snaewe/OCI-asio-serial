#define BOOST_AUTO_TEST_MAIN

#include <boost/test/auto_unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <fstream>
#include "../GPSLib/GPSSentenceDecoder.h"
#include "../GPSLib/Util.h"

/* 
BAD DATA (receiver still initializing):

$GPGGA,000004.000,0000.0000,N,00000.0000,E,0,00,50.0,0.0,M,0.0,M,0.0,0000*72
$GPGLL,36000.0000,N,72000.0000,E,000004.000,V*17
$GPRMC,000003.000,V,36000.0000,N,72000.0000,E,0.000000,,101102,,*3A
$GPGSV,1,1,1,8,0,0,0*40
$GPGSA,A,1,,,,,,,,,,,,,50.0,50.0,50.0*05
*/

BOOST_AUTO_TEST_CASE(InvalidSentenceTest)
{
	const std::string s("$not a valid sentence\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(onInvalidSentenceCalled);
	BOOST_REQUIRE(!onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(!onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}


BOOST_AUTO_TEST_CASE(ChecksumFailedTest)
{
	const std::string s("$GPGSA,A,1,,,,,,,,,,,,,50.0,50.0,50.0*FF\r\n");  // should have checksum 05
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(onInvalidSentenceCalled);
	BOOST_REQUIRE(!onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(!onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}



BOOST_AUTO_TEST_CASE(UninitializedGGATest)
{
	const std::string s("$GPGGA,000004.000,0000.0000,N,00000.0000,E,0,00,50.0,0.0,M,0.0,M,0.0,0000*72\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
		BOOST_REQUIRE_EQUAL(boost::posix_time::duration_from_string("00:00:4.000"), time);
		BOOST_REQUIRE_CLOSE(0, latitude, 0.01);
		BOOST_REQUIRE_CLOSE(0, longitude, 0.01);
		BOOST_REQUIRE_EQUAL(0, quality);
		BOOST_REQUIRE_EQUAL(0, numSatellites);
		BOOST_REQUIRE_CLOSE(50.0, horizontalDilution, 0.01);
		BOOST_REQUIRE_CLOSE(0, altitude, 0.01);
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(!onInvalidSentenceCalled);
	BOOST_REQUIRE(onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(!onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}


// ensure a sentence can arrive in pieces and still be parsed
BOOST_AUTO_TEST_CASE(SeparatedUninitializedGGATest)
{
	const std::string s1("$GPGGA,000004.000,0000.0000,N,00");
	const std::string s2("000.0000,E,0,00,50.0,0.0,M,0.0,M,0.0,0000*72\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
			BOOST_REQUIRE_EQUAL(boost::posix_time::duration_from_string("00:00:4.000"), time);
			BOOST_REQUIRE_CLOSE(0, latitude, 0.01);
			BOOST_REQUIRE_CLOSE(0, longitude, 0.01);
			BOOST_REQUIRE_EQUAL(0, quality);
			BOOST_REQUIRE_EQUAL(0, numSatellites);
			BOOST_REQUIRE_CLOSE(50.0, horizontalDilution, 0.01);
			BOOST_REQUIRE_CLOSE(0, altitude, 0.01);
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s1.begin(), s1.end()));
	d->AddBytes(ios, std::vector<unsigned char>(s2.begin(), s2.end()));

	// under Linux, can't call run(), then run() again - post doesn't get delivered
	ios.run();

	BOOST_REQUIRE(!onInvalidSentenceCalled);
	BOOST_REQUIRE(onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(!onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}



BOOST_AUTO_TEST_CASE(UninitializedGLLTest)
{
	const std::string s("$GPGLL,36000.0000,N,72000.0000,E,000004.000,V*17\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
		BOOST_REQUIRE_EQUAL(boost::posix_time::duration_from_string("00:00:4.000"), time);
		BOOST_REQUIRE_CLOSE(360, latitude, 0.01);
		BOOST_REQUIRE_CLOSE(720, longitude, 0.01);
		BOOST_REQUIRE_EQUAL("V", validity);
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(!onInvalidSentenceCalled);
	BOOST_REQUIRE(!onGGACalled); 
	BOOST_REQUIRE(onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(!onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}




BOOST_AUTO_TEST_CASE(UninitializedRMCTest)
{
	const std::string s("$GPRMC,000003.000,V,36000.0000,N,72000.0000,E,0.000000,,101102,,*3A\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
		BOOST_REQUIRE_EQUAL(boost::posix_time::duration_from_string("00:00:3.000"), time);
		BOOST_REQUIRE_CLOSE(360, latitude, 0.01);
		BOOST_REQUIRE_CLOSE(720, longitude, 0.01);
		BOOST_REQUIRE_EQUAL(boost::gregorian::date(
			boost::gregorian::date::year_type(2002), 
			boost::gregorian::date::month_type(11),
			boost::gregorian::date::day_type(10)), date);
		BOOST_REQUIRE_EQUAL("V", validity);
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(!onInvalidSentenceCalled);
	BOOST_REQUIRE(!onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(onRMCCalled); 
	BOOST_REQUIRE(!onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}


BOOST_AUTO_TEST_CASE(UninitializedGSVTest)
{
	const std::string s("$GPGSV,1,1,1,8,0,0,0*40\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
		BOOST_REQUIRE_EQUAL(1, totalMessages);
		BOOST_REQUIRE_EQUAL(1, messageNumber);
		BOOST_REQUIRE_EQUAL(1, totalSatellitesInView);
		BOOST_REQUIRE_EQUAL(8, satelliteInfo[0]._prn);
		BOOST_REQUIRE_EQUAL(0, satelliteInfo[0]._elevation);
		BOOST_REQUIRE_EQUAL(0, satelliteInfo[0]._azimuth);
		BOOST_REQUIRE_EQUAL(0, satelliteInfo[0]._snr);
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(!onInvalidSentenceCalled);
	BOOST_REQUIRE(!onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}



BOOST_AUTO_TEST_CASE(UninitializedGSATest)
{
	const std::string s("$GPGSA,A,1,,,,,,,,,,,,,50.0,50.0,50.0*05\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
		BOOST_REQUIRE_EQUAL("A", mode);
		BOOST_REQUIRE_EQUAL(1, fix);
		BOOST_REQUIRE_EQUAL(0, satellitesInView.size());
		BOOST_REQUIRE_CLOSE(50.0, pdop, 0.01);
		BOOST_REQUIRE_CLOSE(50.0, hdop, 0.01);
		BOOST_REQUIRE_CLOSE(50.0, vdop, 0.01);
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(!onInvalidSentenceCalled);
	BOOST_REQUIRE(!onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(!onGSVCalled);
	BOOST_REQUIRE(onGSACalled);
}



/* 
GOOD DATA

$GPGGA,191630.609,3848.2905,N,09018.4239,W,1,06,1.3,132.0,M,-33.7,M,0.0,0000*48
$GPGLL,3848.2905,N,09018.4239,W,191630.609,A*20
$GPGSA,A,3,18,15,29,21,06,09,,,,,,,2.2,1.3,1.8*33
$GPGSV,3,1,10,18,62,311,37,15,47,49,40,14,16,218,30,29,11,186,28*4A
$GPGSV,3,2,10,21,85,224,36,24,34,120,25,23,48,50,0,6,24,295,33*45
$GPGSV,3,3,10,9,34,86,36,137,0,0,0*48
$GPRMC,191630.609,A,3848.2905,N,09018.4239,W,31.464734,56.21,150113,,*14

*/


BOOST_AUTO_TEST_CASE(GGATest)
{
	const std::string s("$GPGGA,191630.609,3848.2905,N,09018.4239,W,1,06,1.3,132.0,M,-33.7,M,0.0,0000*48\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
		BOOST_REQUIRE_EQUAL(boost::posix_time::duration_from_string("19:16:30.609"), time);
		BOOST_REQUIRE_CLOSE(38 +(48.2905/60.0), latitude, 0.01);
		BOOST_REQUIRE_CLOSE(- (90 + (18.4239/60.0)), longitude, 0.01);
		BOOST_REQUIRE_EQUAL(1, quality);
		BOOST_REQUIRE_EQUAL(6, numSatellites);
		BOOST_REQUIRE_CLOSE(1.3, horizontalDilution, 0.01);
		BOOST_REQUIRE_CLOSE(132.0, altitude, 0.01);
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(!onInvalidSentenceCalled);
	BOOST_REQUIRE(onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(!onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}



BOOST_AUTO_TEST_CASE(PartialPlusGGATest)
{
	const std::string s("extracharactershere$GPGGA,191630.609,3848.2905,N,09018.4239,W,1,06,1.3,132.0,M,-33.7,M,0.0,0000*48\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
		BOOST_REQUIRE_EQUAL(boost::posix_time::duration_from_string("19:16:30.609"), time);
		BOOST_REQUIRE_CLOSE(38 +(48.2905/60.0), latitude, 0.01);
		BOOST_REQUIRE_CLOSE(- (90 + (18.4239/60.0)), longitude, 0.01);
		BOOST_REQUIRE_EQUAL(1, quality);
		BOOST_REQUIRE_EQUAL(6, numSatellites);
		BOOST_REQUIRE_CLOSE(1.3, horizontalDilution, 0.01);
		BOOST_REQUIRE_CLOSE(132.0, altitude, 0.01);
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(!onInvalidSentenceCalled);
	BOOST_REQUIRE(onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(!onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}


BOOST_AUTO_TEST_CASE(GSVTest)
{
	const std::string s("$GPGSV,3,1,10,18,62,311,37,15,47,49,40,14,16,218,30,29,11,186,28*4A\r\n");
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false, onGGACalled = false, onGLLCalled = false, onRMCCalled = false,
		onGSVCalled = false, onGSACalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		onInvalidSentenceCalled = true;
	};
	d->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
		int numSatellites, double horizontalDilution, double altitude) {
		onGGACalled = true;
	};
	d->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
		onGLLCalled = true;
	};
	d->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
		double speed, double course, boost::gregorian::date date, const std::string &validity) {
		onRMCCalled = true;
	};
	d->OnGSV = [&](boost::asio::io_service &ios, int totalMessages, int messageNumber, int totalSatellitesInView, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		onGSVCalled = true;
		BOOST_REQUIRE_EQUAL(3, totalMessages);
		BOOST_REQUIRE_EQUAL(1, messageNumber);
		BOOST_REQUIRE_EQUAL(10, totalSatellitesInView);
		BOOST_REQUIRE_EQUAL(4, satelliteInfo.size());
		BOOST_REQUIRE_EQUAL(18, satelliteInfo[0]._prn);
		BOOST_REQUIRE_EQUAL(62, satelliteInfo[0]._elevation);
		BOOST_REQUIRE_EQUAL(311, satelliteInfo[0]._azimuth);
		BOOST_REQUIRE_EQUAL(37, satelliteInfo[0]._snr);
		BOOST_REQUIRE_EQUAL(15, satelliteInfo[1]._prn);
		BOOST_REQUIRE_EQUAL(47, satelliteInfo[1]._elevation);
		BOOST_REQUIRE_EQUAL(49, satelliteInfo[1]._azimuth);
		BOOST_REQUIRE_EQUAL(40, satelliteInfo[1]._snr);
		BOOST_REQUIRE_EQUAL(14, satelliteInfo[2]._prn);
		BOOST_REQUIRE_EQUAL(16, satelliteInfo[2]._elevation);
		BOOST_REQUIRE_EQUAL(218, satelliteInfo[2]._azimuth);
		BOOST_REQUIRE_EQUAL(30, satelliteInfo[2]._snr);
		BOOST_REQUIRE_EQUAL(29, satelliteInfo[3]._prn);
		BOOST_REQUIRE_EQUAL(11, satelliteInfo[3]._elevation);
		BOOST_REQUIRE_EQUAL(186, satelliteInfo[3]._azimuth);
		BOOST_REQUIRE_EQUAL(28, satelliteInfo[3]._snr);
	};
	d->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		onGSACalled = true;
	};

	boost::asio::io_service ios;
	d->AddBytes(ios, std::vector<unsigned char>(s.begin(), s.end()));
	ios.run();

	// ensure proper callback was called (if callback not called, test still would have passed)
	BOOST_REQUIRE(!onInvalidSentenceCalled);
	BOOST_REQUIRE(!onGGACalled); 
	BOOST_REQUIRE(!onGLLCalled); 
	BOOST_REQUIRE(!onRMCCalled); 
	BOOST_REQUIRE(onGSVCalled);
	BOOST_REQUIRE(!onGSACalled);
}



BOOST_AUTO_TEST_CASE(AllSentencesTest)
{
	// read from GPS file, deserialize, pass to decoder, and output all invalid sentences found
	// then fail test if any found

	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> d(new GPSLib::GPSSentenceDecoder);  // so shared_from_this() will work  
	
	bool onInvalidSentenceCalled = false;
	d->OnInvalidSentence = [&](boost::asio::io_service &ios, const std::string &s) {
		std::cerr << "Invalid: >" << s << "<" << std::endl;
		onInvalidSentenceCalled = true;
	};

	// int argc = boost::unit_test::framework::master_test_suite().argc;
	char **argv = boost::unit_test::framework::master_test_suite().argv;

	// EXE is <root>/Output/Debug/<filename> or <root>/Output/Release/<filename>
	// get the parent path 3 times to get <root>
	// add on the path to the data
	boost::filesystem::path dataPath(boost::filesystem::system_complete(argv[0]).parent_path().parent_path().parent_path());
	dataPath /= "Tests";
	dataPath /= "gps_2013-01-15_0106";

	std::ifstream in(dataPath.c_str());
	boost::archive::text_iarchive ia(in);
	uint64_t us;
	boost::asio::io_service ios;
	while (true) {
		try {
			std::vector<unsigned char> buffer;
			ia >> us >> buffer;
			d->AddBytes(ios, buffer);
		} catch (const std::exception &) {
			// end of archive - ignore exception
			break;
		}
	}

	ios.run();
	BOOST_REQUIRE(!onInvalidSentenceCalled);
}
