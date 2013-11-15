#include "../GPSDDSLib/GPSTypeSupportC.h"
#include "../GPSDDSLib/GPSC.h"
#include "../GPSDDSLib/GPSTypeSupportImpl.h"
#include "../GPSDDSLib/DDSException.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>

#include "../ASIOLib/Executor.h"
#include "../ASIOLib/SerialPort.h"
#include "../GPSLib/GPSSentenceDecoder.h"
#include <boost/program_options.hpp>
#include <boost/thread.hpp>


boost::mutex cout_lock;
void Log(const std::string &msg) {
	boost::mutex::scoped_lock lock(cout_lock);
	std::cout << "[" << boost::this_thread::get_id() << "] " << msg << std::endl;
}

class GPSPublisher : public boost::enable_shared_from_this<GPSPublisher> {
	boost::shared_ptr<ASIOLib::SerialPort> _serialPort;
	const std::string _portName;
	const unsigned int _baudRate;
	const boost::shared_ptr<GPSLib::GPSSentenceDecoder> _decoder;  // so shared_from_this() will work  
	const GPS::PositionDataDataWriter_var _positionWriter;
	const GPS::AltitudeDataDataWriter_var _altitudeWriter;
	const GPS::CourseDataDataWriter_var _courseWriter;
	const GPS::SatelliteInfoDataDataWriter_var _satelliteInfoWriter;
	const GPS::ActiveSatellitesDataDataWriter_var _activeSatellitesWriter;
	const boost::posix_time::ptime _epoch;

	std::string GetSensorID() {
		// use machine name/port name as sensor ID
		boost::system::error_code ec;
		std::string hostName = boost::asio::ip::host_name(ec);
		if (ec)
			hostName = "<UNKNOWN>";
		return hostName + ":" + _portName;
	}

	void PublishPosition(boost::posix_time::ptime date, double latitude, double longitude) {
		GPS::PositionData sample;
		sample.sensor_id = GetSensorID().c_str();
		sample.date = (date-_epoch).total_milliseconds();
		sample.latitude = latitude;
		sample.longitude = longitude;

		if (_positionWriter->write(sample, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
			throw DDSException("position write() failed");
	}

	void PublishAltitude(boost::posix_time::ptime date, double altitude) {
		GPS::AltitudeData sample;
		sample.sensor_id = GetSensorID().c_str();
		sample.date = (date-_epoch).total_milliseconds();
		sample.altitude = altitude;

		if (_altitudeWriter->write(sample, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
			throw DDSException("altitude write() failed");
	}

	void PublishCourse(boost::posix_time::ptime date, double speed, double course) {
		GPS::CourseData sample;
		sample.sensor_id = GetSensorID().c_str();
		sample.date = (date-_epoch).total_milliseconds();
		sample.speed = speed;
		sample.course = course;

		if (_courseWriter->write(sample, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
			throw DDSException("course write() failed");
	}

	void PublishSatelliteInfo(const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
		GPS::SatelliteInfoData sample;
		sample.sensor_id = GetSensorID().c_str();
		sample.satelliteInfo.length(satelliteInfo.size());
		for (size_t i=0; i<satelliteInfo.size(); i++) {
			sample.satelliteInfo[i].prn = satelliteInfo[i]._prn;
			sample.satelliteInfo[i].azimuth = satelliteInfo[i]._azimuth;
			sample.satelliteInfo[i].elevation = satelliteInfo[i]._elevation;
			sample.satelliteInfo[i].snr = satelliteInfo[i]._snr;
		}

		if (_satelliteInfoWriter->write(sample, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
			throw DDSException("satelliteInfo write() failed");
	}

	void PublishActiveSatellites(const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
		GPS::ActiveSatellitesData sample;
		sample.sensor_id = GetSensorID().c_str();
		sample.activeSatellites.length(satellitesInView.size());
		for (size_t i=0; i<satellitesInView.size(); i++) 
			sample.activeSatellites[i] = satellitesInView[i];
		sample.pdop = pdop;
		sample.hdop = hdop;
		sample.vdop = vdop;

		if (_activeSatellitesWriter->write(sample, DDS::HANDLE_NIL) != DDS::RETCODE_OK)
			throw DDSException("activeSatellitesInfo write() failed");
	}


	void OnRead(boost::asio::io_service &ios, const std::vector<unsigned char> &buffer, size_t bytesRead) {
		// GPS decode here
		_decoder->OnGGA = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, int quality, 
			int /*numSatellites*/, double /*horizontalDilution*/, double altitude) {
			if (quality != 0) { // only post if valid
				PublishPosition(boost::posix_time::ptime(_epoch + time), latitude, longitude);
				PublishAltitude(boost::posix_time::ptime(_epoch + time), altitude);
			}
		};
		_decoder->OnGLL = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude, const std::string &validity) {
			if (validity == "A")  // only post if valid
				PublishPosition(boost::posix_time::ptime(_epoch + time), latitude, longitude);
		};
		_decoder->OnRMC = [&](boost::asio::io_service &ios, boost::posix_time::time_duration time, double latitude, double longitude,
			double speed, double course, boost::gregorian::date date, const std::string &validity) {
			if (validity == "A") {  // only post if valid
				PublishPosition(boost::posix_time::ptime(date, time), latitude, longitude);
				PublishCourse(boost::posix_time::ptime(date, time), speed, course);
			}
		};
		_decoder->OnGSV = [&](boost::asio::io_service &ios, int /*totalMessages*/, int /*messageNumber*/, int /*totalSatellitesInView*/, const std::vector<GPSLib::SatelliteInfo> &satelliteInfo) {
			PublishSatelliteInfo(satelliteInfo);
		};
		_decoder->OnGSA = [&](boost::asio::io_service &ios, const std::string &mode, int fix, const std::vector<int> &satellitesInView, double pdop, double hdop, double vdop) {
			PublishActiveSatellites(satellitesInView, pdop, hdop, vdop);
		};


		_decoder->AddBytes(ios, std::vector<unsigned char>(buffer.begin(), buffer.begin()+bytesRead));
	}

public:
	GPSPublisher(const std::string &portName, int baudRate, GPS::PositionDataDataWriter_ptr positionWriter, GPS::AltitudeDataDataWriter_ptr altitudeWriter,
		GPS::CourseDataDataWriter_ptr courseWriter, GPS::SatelliteInfoDataDataWriter_ptr satelliteInfoWriter, GPS::ActiveSatellitesDataDataWriter_ptr activeSatellitesWriter) : 
	_epoch(boost::gregorian::date(1970, 1, 1)),
	  _portName(portName), _baudRate(baudRate), _positionWriter(positionWriter), _altitudeWriter(altitudeWriter),
		  _courseWriter(courseWriter), _satelliteInfoWriter(satelliteInfoWriter), _activeSatellitesWriter(activeSatellitesWriter),
		  _decoder(new GPSLib::GPSSentenceDecoder) {}
	void Create(boost::asio::io_service &ios) {
		try {
			_serialPort.reset(new ASIOLib::SerialPort(ios,  _portName));  
			_serialPort->Open(boost::bind(&GPSPublisher::OnRead, shared_from_this(), _1, _2, _3), _baudRate);
		} catch (const std::exception &e) {
			std::cout << "GPSPublisher exception (create): " << e.what() << std::endl;		
		}
	}
};


template <typename TTypeSupport, typename TTypeSupportImpl, typename TDataWriter>
TAO_Objref_Var_T<TDataWriter> CreateWriter(DDS::DomainParticipant_ptr dp, const char *topicName) {
	TAO_Objref_Var_T<TTypeSupport> ts = new TTypeSupportImpl;
	if (ts->register_type(dp, "") != DDS::RETCODE_OK)
		throw DDSException("reigster_type() failed");

	CORBA::String_var typeName(ts->get_type_name());
	DDS::Topic_var topic = dp->create_topic(topicName, typeName, TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
	if (0 == topic) 
		throw DDSException("create_topic() failed");

	DDS::Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
	if (0 == pub) 
		throw DDSException("create_publisher() failed");

	DDS::DataWriterQos dw_qos;
	pub->get_default_datawriter_qos(dw_qos);
	DDS::DataWriter_var dw = pub->create_datawriter(topic, dw_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
	if (0 == dw) 
		throw DDSException("create_datawriter() failed");

	TAO_Objref_Var_T<TDataWriter> ndw = TDataWriter::_narrow(dw);
	if (0 == ndw) 
		throw DDSException("writer _narrow() failed");

	return ndw;
}

#define CREATE_WRITER(X) CreateWriter<X##TypeSupport, X##TypeSupportImpl, X##DataWriter>

int main(int argc, char *argv[]) {
	DDS::DomainParticipantFactory_var dpf;
	DDS::DomainParticipant_var dp;
	try {
	  std::string portName, file;
		int baudRate;
		boost::program_options::options_description desc("Options");
		desc.add_options()
			("help,h", "help")
			("port,p", boost::program_options::value<std::string>(&portName)->required(), "port name (required)")
			("baud,b", boost::program_options::value<int>(&baudRate)->required(), "baud rate (required)")
			;
	
		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
		
		if (vm.empty() || vm.count("help")) {
			std::cout << desc << "\n";
			return -1;
		}

		boost::program_options::notify(vm);

		dpf = TheParticipantFactoryWithArgs(argc, argv);
		dp = dpf->create_participant(42, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
		if (0 == dp) 
			throw DDSException("create_participant() failed");

		const GPS::PositionDataDataWriter_var positionWriter = CREATE_WRITER(GPS::PositionData)(dp, "GPS_Position");
		const GPS::AltitudeDataDataWriter_var altitudeWriter = CREATE_WRITER(GPS::AltitudeData)(dp, "GPS_Altitude");
		const GPS::CourseDataDataWriter_var courseWriter = CREATE_WRITER(GPS::CourseData)(dp, "GPS_Course");
		const GPS::SatelliteInfoDataDataWriter_var satelliteInfoWriter = CREATE_WRITER(GPS::SatelliteInfoData)(dp, "GPS_SatelliteInfo");
		const GPS::ActiveSatellitesDataDataWriter_var activeSatellitesInfoWriter = CREATE_WRITER(GPS::ActiveSatellitesData)(dp, "GPS_ActiveSatellites");

		ASIOLib::Executor e;
		e.OnWorkerThreadError = [](boost::asio::io_service &, boost::system::error_code ec) { Log(std::string("GPSPublisher error (asio): ") + boost::lexical_cast<std::string>(ec)); };
		e.OnWorkerThreadException = [](boost::asio::io_service &, const std::exception &ex) { Log(std::string("GPSPublisher exception (asio): ") + ex.what()); };

		boost::shared_ptr<GPSPublisher> spd(new GPSPublisher(portName, baudRate, positionWriter, altitudeWriter, courseWriter, satelliteInfoWriter, activeSatellitesInfoWriter));  // for shared_from_this() to work inside of Reader, Reader must already be managed by a smart pointer
		e.OnRun = boost::bind(&GPSPublisher::Create, spd, _1);
		e.Run();
	} catch (const std::exception &e) {
		std::cout << "GPSPublisher exception (main): " << e.what() << std::endl;
		return -1;
	} catch (const CORBA::Exception &e) {
		e._tao_print_exception("GPSPublisher exception (main): ");
		return -1;
	} 

	if (0 != dp)
		dp->delete_contained_entities();
	if (0 != dpf)
		dpf->delete_participant(dp);

	TheServiceParticipant->shutdown();

	return 0;
}
