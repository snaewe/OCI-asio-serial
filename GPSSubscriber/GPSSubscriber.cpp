// #include "../GPSDDSLib/GPSTypeSupportC.h"
#include "../GPSDDSLib/GPSTypeSupportImpl.h"
#include "../GPSDDSLib/DDSException.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <iostream>
#include <boost/date_time.hpp>
#include <boost/tuple/tuple.hpp>


class DataReaderListenerImpl
	: public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
	virtual void on_requested_deadline_missed(
		DDS::DataReader_ptr reader,
		const DDS::RequestedDeadlineMissedStatus& status) {}

	virtual void on_requested_incompatible_qos(
		DDS::DataReader_ptr reader,
		const DDS::RequestedIncompatibleQosStatus& status) {}

	virtual void on_sample_rejected(
		DDS::DataReader_ptr reader,
		const DDS::SampleRejectedStatus& status) {}

	virtual void on_liveliness_changed(
		DDS::DataReader_ptr reader,
		const DDS::LivelinessChangedStatus& status) {}

	virtual void on_data_available(
		DDS::DataReader_ptr reader);

	virtual void on_subscription_matched(
		DDS::DataReader_ptr reader,
		const DDS::SubscriptionMatchedStatus& status) {}

	virtual void on_sample_lost(
		DDS::DataReader_ptr reader,
		const DDS::SampleLostStatus& status) {}
};


void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader) {
	const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
	DDS::SampleInfo info;

	GPS::PositionDataDataReader_var positionReader =
		GPS::PositionDataDataReader::_narrow(reader);

	if (positionReader) {
		GPS::PositionData sample;
		DDS::ReturnCode_t error = positionReader->take_next_sample(sample, info);
		if ((error == DDS::RETCODE_OK) && info.valid_data) {
			boost::posix_time::ptime time(epoch + boost::posix_time::time_duration(boost::posix_time::milliseconds(sample.date)));
			std::cout << "Position: " << sample.sensor_id << " " << time << " " << sample.latitude << " " << sample.longitude << std::endl;
		}
		return;
	}


	GPS::AltitudeDataDataReader_var altitudeReader =
		GPS::AltitudeDataDataReader::_narrow(reader);

	if (altitudeReader) {
		GPS::AltitudeData sample;
		DDS::ReturnCode_t error = altitudeReader->take_next_sample(sample, info);
		if ((error == DDS::RETCODE_OK) && info.valid_data) {
			boost::posix_time::ptime time(epoch + boost::posix_time::time_duration(boost::posix_time::milliseconds(sample.date)));
			std::cout << "Altitude: " << sample.sensor_id << " " << time << " " << sample.altitude << std::endl;
		}
		return;
	}


	GPS::CourseDataDataReader_var courseReader =
		GPS::CourseDataDataReader::_narrow(reader);

	if (courseReader) {
		GPS::CourseData sample;
		DDS::ReturnCode_t error = courseReader->take_next_sample(sample, info);
		if ((error == DDS::RETCODE_OK) && info.valid_data) {
			boost::posix_time::ptime time(epoch + boost::posix_time::time_duration(boost::posix_time::milliseconds(sample.date)));
			std::cout << "Course: " << sample.sensor_id << " " << time << " " << sample.speed << " " << sample.course << std::endl;
		}
		return;
	}


	GPS::SatelliteInfoDataDataReader_var satelliteInfoReader =
		GPS::SatelliteInfoDataDataReader::_narrow(reader);

	if (satelliteInfoReader) {
		GPS::SatelliteInfoData sample;
		DDS::ReturnCode_t error = satelliteInfoReader->take_next_sample(sample, info);
		if ((error == DDS::RETCODE_OK) && info.valid_data) {
			std::cout << "Satellite info: " << sample.sensor_id << " ";
			for (CORBA::ULong i=0; i<sample.satelliteInfo.length(); i++) {
				std::cout << "[" << sample.satelliteInfo[i].prn << "," << sample.satelliteInfo[i].azimuth << "," << sample.satelliteInfo[i].elevation <<
					"," << sample.satelliteInfo[i].snr << "]";
			}
			std::cout << std::endl;
		}
		return;
	}

	GPS::ActiveSatellitesDataDataReader_var activeSatellitesReader =
		GPS::ActiveSatellitesDataDataReader::_narrow(reader);

	if (activeSatellitesReader) {
		GPS::ActiveSatellitesData sample;
		DDS::ReturnCode_t error = activeSatellitesReader->take_next_sample(sample, info);
		if ((error == DDS::RETCODE_OK) && info.valid_data) {
			std::cout << "Active Satellites: " << sample.sensor_id << " " << sample.pdop << " " << sample.hdop << " " << 
				sample.vdop << " [";
			for (CORBA::ULong i=0; i<sample.activeSatellites.length(); i++)
				std::cout << sample.activeSatellites[i] << " ";
			std::cout << "]" << std::endl;
		}
		return;
	}

}



template <typename TTypeSupport, typename TTypeSupportImpl, typename TDataReader>
TAO_Objref_Var_T<TDataReader> CreateReader(DDS::DomainParticipant_ptr dp, DDS::DataReaderListener_ptr listener, const char *topicName) {
	TAO_Objref_Var_T<TTypeSupport> ts = new TTypeSupportImpl;
	if (ts->register_type(dp, "") != DDS::RETCODE_OK)
		throw DDSException("reigster_type() failed");

	CORBA::String_var typeName(ts->get_type_name());
	DDS::Topic_var topic = dp->create_topic(topicName, typeName, TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
	if (0 == topic) 
		throw DDSException("create_topic() failed");

	DDS::Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
	if (0 == sub) 
		throw DDSException("create_subscriber() failed");

	DDS::DataReaderQos dr_qos;
	sub->get_default_datareader_qos(dr_qos);
	DDS::DataReader_var dr = sub->create_datareader(topic, dr_qos, listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
	if (0 == dr) 
		throw DDSException("create_datareader() failed");

	TAO_Objref_Var_T<TDataReader> ndr = TDataReader::_narrow(dr);
	if (0 == ndr) 
		throw DDSException("reader _narrow() failed");

	return ndr;
}


void WaitForPublisherToComplete(DDS::DataReader_ptr dr) {
	DDS::StatusCondition_var condition = dr->get_statuscondition();
	condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

	DDS::WaitSet_var ws = new DDS::WaitSet;
	ws->attach_condition(condition);

	while (true) {
		DDS::SubscriptionMatchedStatus matches;
		if (dr->get_subscription_matched_status(matches) != DDS::RETCODE_OK) 
			throw DDSException("get_subscription_matched_status() failed");

		if (matches.current_count == 0 && matches.total_count > 0)
			break;

		DDS::ConditionSeq conditions;
		DDS::Duration_t timeout = { 60, 0 };
		if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) 
			throw DDSException("wait() failed");
	}

	ws->detach_condition(condition);
}


#define CREATE_READER(X) CreateReader<X##TypeSupport, X##TypeSupportImpl, X##DataReader>

int main(int argc, char *argv[]) {
	DDS::DomainParticipantFactory_var dpf;
	DDS::DomainParticipant_var dp;
	try {
		dpf = TheParticipantFactoryWithArgs(argc, argv);
		dp = dpf->create_participant(42, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
		if (0 == dp) 
			throw DDSException("create_participant() failed");

		DDS::DataReaderListener_var listener(new DataReaderListenerImpl);
		const GPS::PositionDataDataReader_var positonReader = CREATE_READER(GPS::PositionData)(dp, listener, "GPS_Position");
		const GPS::AltitudeDataDataReader_var altitudeReader =  CREATE_READER(GPS::AltitudeData)(dp, listener, "GPS_Altitude");
		const GPS::CourseDataDataReader_var courseReader = CREATE_READER(GPS::CourseData)(dp, listener, "GPS_Course");
		const GPS::SatelliteInfoDataDataReader_var satelliteInfoReader = CREATE_READER(GPS::SatelliteInfoData)(dp, listener, "GPS_SatelliteInfo");
		const GPS::ActiveSatellitesDataDataReader_var activeSatellitesInfoReader = CREATE_READER(GPS::ActiveSatellitesData)(dp, listener, "GPS_ActiveSatellites");

		WaitForPublisherToComplete(positonReader);
		WaitForPublisherToComplete(altitudeReader);
		WaitForPublisherToComplete(courseReader);
		WaitForPublisherToComplete(satelliteInfoReader);
		WaitForPublisherToComplete(activeSatellitesInfoReader);

	} catch (const std::exception &e) {
		std::cout << "GPSSubscriber exception: " << e.what() << std::endl;
		return -1;
	} catch (const CORBA::Exception &e) {
		e._tao_print_exception("GPSSubscriber exception: ");
		return -1;
	} 

	if (0 != dp)
		dp->delete_contained_entities();
	if (0 != dpf)
		dpf->delete_participant(dp);

	TheServiceParticipant->shutdown();

	return 0;
}
