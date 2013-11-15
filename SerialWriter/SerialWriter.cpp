#include "../ASIOLib/Executor.h"
#include "../ASIOLib/SerialPort.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>


boost::mutex cout_lock;
void Log(const std::string &msg) {
	boost::mutex::scoped_lock lock(cout_lock);
	std::cout << "[" << boost::this_thread::get_id() << "] " << msg << std::endl;
}


typedef boost::tuple<boost::posix_time::time_duration::tick_type, std::vector<unsigned char>> WriteBufferElement;

class SerialWriter : private boost::noncopyable, public boost::enable_shared_from_this<SerialWriter> {
	boost::shared_ptr<ASIOLib::SerialPort> _serialPort;
	std::string _portName;
	unsigned int _baudRate;
	std::vector<WriteBufferElement> _writeBuffer;
	bool _noTimeOffsets;
public:
	SerialWriter(const std::string &portName, int baudRate, const std::vector<WriteBufferElement> &writeBuffer, bool noTimeOffsets) :
	  _portName(portName), _baudRate(baudRate), _writeBuffer(writeBuffer), _noTimeOffsets(noTimeOffsets) {}

	void Create(boost::asio::io_service &ios) {
		_serialPort.reset(new ASIOLib::SerialPort(ios,  _portName));  
		_serialPort->Open(0, _baudRate);

		// post the creation, so Run() executes immediately - threads can start to work on what the func is posting, rather than waiting until all work is queued
		ios.post([=, &ios] {
			uint64_t startTime = 0;
			std::for_each(_writeBuffer.begin(), _writeBuffer.end(), 
				[&](const WriteBufferElement &e) {
					// if noTimeOffsets, just write the buffer, otherwise create a timer to write the buffer in the future
					if (_noTimeOffsets)
						_serialPort->Write(e.get<1>());
					else {
						startTime += e.get<0>();
						const boost::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(ios));
						timer->expires_from_now(boost::posix_time::milliseconds(startTime));
						timer->async_wait([=](const boost::system::error_code &ec) { 
							boost::shared_ptr<boost::asio::deadline_timer> t(timer); // need this to keep the timer object alive
							_serialPort->Write(e.get<1>()); 
						});
					}
				}
			);
		});
	}
};



int main(int argc, char *argv[]) {
	try {
		std::string portName, message, file;
		int baudRate;
		bool noTimeOffsets = false;

		boost::program_options::options_description desc("Options");
		desc.add_options()
			("help,h", "help")
			("port,p", boost::program_options::value<std::string>(&portName)->required(), "port name (required)")
			("baud,b", boost::program_options::value<int>(&baudRate)->required(), "baud rate (required)")
			("message,m", boost::program_options::value<std::string>(&message), "message to send")
			("file,f", boost::program_options::value<std::string>(&file), "file to send")
			("no_time_offsets,n", boost::program_options::value<bool>(&noTimeOffsets)->zero_tokens(), "ignore time offsets in files to send data as fast as possible")
			;
	
		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

		if (vm.empty() || vm.count("help")) {
			std::cout << desc << "\n";
			return -1;
		}

		boost::program_options::notify(vm);  

		std::vector<WriteBufferElement> writeBuffer;

		if (!message.empty()) {
			std::vector<unsigned char> v;
			std::copy(message.begin(), message.end(), std::back_insert_iterator<std::vector<unsigned char>>(v));
			writeBuffer.push_back(boost::make_tuple(0, v));
		}

		if (!file.empty()) {
			std::ifstream in(file);
			boost::archive::text_iarchive ia(in);
			
			while (true) {
				try {
					WriteBufferElement e;
					uint64_t us;
					ia >> us >> e.get<1>();
					e.get<0>() = us;
					writeBuffer.push_back(e);
				} catch (const std::exception &) {
					// end of archive - ignore exception
					break;
				}
			}
		}

		ASIOLib::Executor e;
		e.OnWorkerThreadError = [](boost::asio::io_service &, boost::system::error_code ec) { Log(std::string("SerialWriter error (asio): ") + boost::lexical_cast<std::string>(ec)); };
		e.OnWorkerThreadException = [](boost::asio::io_service &, const std::exception &ex) { Log(std::string("SerialWriter exception (asio): ") + ex.what()); };
		const boost::shared_ptr<SerialWriter> spd(new SerialWriter(portName, baudRate, writeBuffer, noTimeOffsets));
		e.OnRun = boost::bind(&SerialWriter::Create, spd, _1);
		e.Run();
	} catch (const std::exception &e) {
		std::cout << "SerialWriter exception (main): " << e.what() << std::endl;
		return -1;
	}
	return 0;
}

