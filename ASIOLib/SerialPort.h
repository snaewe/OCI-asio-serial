#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include "ASIOLib_Export.h"

namespace ASIOLib {
	typedef boost::tuple<
		boost::asio::serial_port_base::character_size,
		boost::asio::serial_port_base::parity,
		boost::asio::serial_port_base::stop_bits> SerialParams;

	const SerialParams SP_8N1 = boost::make_tuple(
		8,
		boost::asio::serial_port_base::parity::none,
		boost::asio::serial_port_base::stop_bits::one);

	const SerialParams SP_7E1 = boost::make_tuple(
		7,
		boost::asio::serial_port_base::parity::even,
		boost::asio::serial_port_base::stop_bits::one);

	class ASIOLib_Export SerialPort : private boost::noncopyable, public boost::enable_shared_from_this<SerialPort>  {
		boost::asio::serial_port _serialPort;
		bool _isOpen;
		boost::system::error_code _errorCode;
		boost::mutex _errorCodeMutex;
		std::vector<unsigned char> _readBuffer;
		boost::function<void (boost::asio::io_service &, const std::vector<unsigned char> &, size_t)> _onRead;

		std::vector<unsigned char> _writeQueue, _writeBuffer;
		boost::mutex _writeQueueMutex, _writeBufferMutex;

		boost::system::error_code Flush();
		void SetErrorCode(const boost::system::error_code &ec);
		void ReadBegin();
		void ReadComplete(const boost::system::error_code &ec, size_t bytesTransferred);
		void WriteBegin();
		void WriteComplete(const boost::system::error_code &ec);
	public:
		SerialPort(boost::asio::io_service &ioService, const std::string& portName);
		~SerialPort();
		void Open(
			const boost::function<void (boost::asio::io_service &, const std::vector<unsigned char> &, size_t)> &onRead,
			unsigned int baudRate,
			SerialParams serialParams = SP_8N1,
			boost::asio::serial_port_base::flow_control flowControl =
				boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none)		
			);
		void Close();
		void Write(const unsigned char *buffer, size_t bufferLength);
		void Write(const std::vector<unsigned char> &buffer);
		void Write(const std::string &buffer);
	};
};

#endif
