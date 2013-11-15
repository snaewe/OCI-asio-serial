#include "SerialPort.h"
#include <boost/bind.hpp>

// http://www.progtown.com/topic90228-how-for-boost-asio-serialport-to-make-flush.html

boost::system::error_code ASIOLib::SerialPort::Flush() {
	boost::system::error_code ec;
#if !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
	const bool isFlushed = !::tcflush(_serialPort.native(), TCIOFLUSH);
	if (!isFlushed)
		ec = boost::system::error_code(errno, boost::asio::error::get_system_category());
#else
	const bool isFlushed = ::PurgeComm(_serialPort.native(), PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
	if (!isFlushed)
		ec = boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category()); 
#endif 
	return ec;
}

void ASIOLib::SerialPort::SetErrorCode(const boost::system::error_code &ec) {
	if (ec) {
		boost::mutex::scoped_lock lock(_errorCodeMutex);
		_errorCode = ec;
	}
}

void ASIOLib::SerialPort::ReadBegin() {
	_serialPort.async_read_some(boost::asio::buffer(_readBuffer),
		boost::bind(&SerialPort::ReadComplete, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void ASIOLib::SerialPort::ReadComplete(const boost::system::error_code &ec, size_t bytesTransferred) {
	if (!ec) {
		if (_onRead && (bytesTransferred > 0))
			_onRead(boost::ref(_serialPort.get_io_service()), boost::cref(_readBuffer), bytesTransferred); // callback executes before any additional reads are queued, so access to the buffer is guaranteed as the buffer won't be overwritten
		ReadBegin();  // queue another read
	} else {
		Close();
		SetErrorCode(ec);
	}
}


void ASIOLib::SerialPort::WriteBegin() {
	boost::mutex::scoped_lock writeBufferlock(_writeBufferMutex);
	if (_writeBuffer.size() != 0)
		return;  // a write is in progress, so don't start another

	boost::mutex::scoped_lock writeQueuelock(_writeQueueMutex);
	if (_writeQueue.size() == 0)
		return;  // nothing to write

	// allocate a larger buffer if needed
	const std::vector<unsigned char>::size_type writeQueueSize = _writeQueue.size();
	if (writeQueueSize > _writeBuffer.size()) 
		_writeBuffer.resize(writeQueueSize);
	// resize, not reserve, as copy can't create elements - only copy over existing ones

	// copy the queued bytes to the write buffer, and clear the queued bytes
	std::copy(_writeQueue.begin(), _writeQueue.end(), _writeBuffer.begin());
	_writeQueue.clear();

	boost::asio::async_write(_serialPort, boost::asio::buffer(_writeBuffer, writeQueueSize),
		boost::bind(&SerialPort::WriteComplete, shared_from_this(), boost::asio::placeholders::error));
}

void ASIOLib::SerialPort::WriteComplete(const boost::system::error_code &ec) {
	if (!ec) {
		{
			// everything in the buffer was sent, so set the length to 0 so WriteBegin knows a write is no longer in progress
			boost::mutex::scoped_lock lock(_writeBufferMutex);
			_writeBuffer.clear();
		}
		WriteBegin();  // more bytes to send may have arrived while the write was in progress, so check again
	} else {
		Close();
		SetErrorCode(ec);
	}
}


ASIOLib::SerialPort::SerialPort(boost::asio::io_service &ioService, const std::string& portName) : 
_serialPort(ioService, portName), _isOpen(false) {
	_readBuffer.resize(128);
}

void ASIOLib::SerialPort::Open(
	const boost::function<void (boost::asio::io_service &, const std::vector<unsigned char> &, size_t)> &onRead,
	unsigned int baudRate,
	ASIOLib::SerialParams serialParams,
	boost::asio::serial_port_base::flow_control flowControl) {
		_onRead = onRead;
		_serialPort.set_option(boost::asio::serial_port_base::baud_rate(baudRate));
		_serialPort.set_option(serialParams.get<0>());
		_serialPort.set_option(serialParams.get<1>());
		_serialPort.set_option(serialParams.get<2>());
		_serialPort.set_option(flowControl);

		const boost::system::error_code ec = Flush();
		if (ec)
			SetErrorCode(ec);

		_isOpen = true;

		if (_onRead) {
			// don't start the async reader unless a read callback has been provided
			// be sure shared_from_this() is only called after object is already managed in a shared_ptr, so need to fully construct, then call Open() on the ptr
			_serialPort.get_io_service().post(boost::bind(&SerialPort::ReadBegin, shared_from_this()));  // want read to start from a thread in io_service
		}
}

ASIOLib::SerialPort::~SerialPort() {
	Close();
}

void ASIOLib::SerialPort::Close() {
	if (_isOpen) {
		_isOpen = false;
		boost::system::error_code ec;
		_serialPort.cancel(ec);
		SetErrorCode(ec);
		_serialPort.close(ec);
		SetErrorCode(ec);
	}
}

void ASIOLib::SerialPort::Write(const unsigned char *buffer, size_t bufferLength) {
	{
		boost::mutex::scoped_lock lock(_writeQueueMutex);
		_writeQueue.insert(_writeQueue.end(), buffer, buffer+bufferLength);
	}
	_serialPort.get_io_service().post(boost::bind(&SerialPort::WriteBegin, shared_from_this()));
}

void ASIOLib::SerialPort::Write(const std::vector<unsigned char> &buffer) {
	Write(&buffer[0], buffer.size());
}

void ASIOLib::SerialPort::Write(const std::string &buffer) {
	Write(reinterpret_cast<const unsigned char *>(buffer.c_str()), buffer.size());
}


