#include "../ASIOLib/Executor.h"
#include "../ASIOLib/SerialPort.h"
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/program_options.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>

boost::mutex cout_lock;
void Log(const std::string &msg) {
	boost::mutex::scoped_lock lock(cout_lock);
	std::cout << "[" << boost::this_thread::get_id() << "] " << msg << std::endl;
}


class SerialReader : private boost::noncopyable, public boost::enable_shared_from_this<SerialReader> {
	boost::shared_ptr<ASIOLib::SerialPort> _serialPort;
	std::string _portName;
	unsigned int _baudRate;
	const boost::scoped_ptr<boost::archive::text_oarchive> &_oa;
	boost::posix_time::ptime _lastRead;

	void OnRead(boost::asio::io_service &ios, const std::vector<unsigned char> &buffer, size_t bytesRead);
	
public:
	SerialReader(const std::string &portName, int baudRate, const boost::scoped_ptr<boost::archive::text_oarchive> &oa) : 
	  _portName(portName), _baudRate(baudRate), _oa(oa) {}
	void Create(boost::asio::io_service &ios) {
		try {
			_serialPort.reset(new ASIOLib::SerialPort(ios,  _portName));  
			_serialPort->Open(boost::bind(&SerialReader::OnRead, shared_from_this(), _1, _2, _3), _baudRate);
		} catch (const std::exception &e) {
			std::cout << e.what() << std::endl;		
		}
	}
};


void SerialReader::OnRead(boost::asio::io_service &, const std::vector<unsigned char> &buffer, size_t bytesRead) {
	const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();

	if (_lastRead == boost::posix_time::not_a_date_time)
		_lastRead = now;

	const std::vector<unsigned char> v(buffer.begin(), buffer.begin()+bytesRead);

	if (_oa) { 		
		const uint64_t offset = (now-_lastRead).total_milliseconds();
		*_oa << offset << v;
	}
	_lastRead = now;

	std::copy(v.begin(), v.end(), std::ostream_iterator<unsigned char>(std::cout, ""));
}


int main(int argc, char *argv[]) {
	std::string portName, file;
	int baudRate;
	boost::program_options::options_description desc("Options");
	desc.add_options()
		("help,h", "help")
		("port,p", boost::program_options::value<std::string>(&portName)->required(), "port name (required)")
		("baud,b", boost::program_options::value<int>(&baudRate)->required(), "baud rate (required)")
		("file,f", boost::program_options::value<std::string>(&file), "file to save to")
		;
	
	boost::program_options::variables_map vm;
	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

	if (vm.empty() || vm.count("help")) {
		std::cout << desc << "\n";
		return -1;
	}

	// don't call notify() until ready to process errors so help alone doesn't cause an error on missing required parameters
	// http://stackoverflow.com/questions/5395503/required-and-optional-arguments-using-boost-library-program-options
	boost::program_options::notify(vm);  

	const boost::scoped_ptr<std::ostream> out(file.empty() ? 0 : new std::ofstream(file.c_str()));
	const boost::scoped_ptr<boost::archive::text_oarchive> archive(file.empty() ? 0 : new boost::archive::text_oarchive(*out));

	ASIOLib::Executor e;
	e.OnWorkerThreadError = [](boost::asio::io_service &, boost::system::error_code ec) { Log(std::string("SerialReader error (asio): ") + boost::lexical_cast<std::string>(ec)); };
	e.OnWorkerThreadException = [](boost::asio::io_service &, const std::exception &ex) { Log(std::string("SerialReader exception (asio): ") + ex.what()); };

	const boost::shared_ptr<SerialReader> sp(new SerialReader(portName, baudRate, archive));  // for shared_from_this() to work inside of Reader, Reader must already be managed by a smart pointer
	e.OnRun = boost::bind(&SerialReader::Create, sp, _1);
	e.Run();

	return 0;
}
