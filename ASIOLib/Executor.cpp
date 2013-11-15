#include "Executor.h"
#include <boost/thread.hpp>

void ASIOLib::Executor::WorkerThread(boost::asio::io_service &ios) {
	if (OnWorkerThreadStart)
		OnWorkerThreadStart(ios);

	while (true) {
		try
		{
			boost::system::error_code ec;
			ios.run(ec);
			if (ec && OnWorkerThreadError)
				OnWorkerThreadError(ios, ec);
			break;
		}
		catch (const std::exception &ex) {
			if (OnWorkerThreadException)
				OnWorkerThreadException(ios, ex);
		}
	}

	if (OnWorkerThreadStop)
		OnWorkerThreadStop(ios);
}

void ASIOLib::Executor::AddCtrlCHandling() {
	// stop when ctrl-c is pressed
	boost::asio::signal_set sig_set(_ioService, SIGTERM, SIGINT);
	sig_set.async_wait(boost::bind(&boost::asio::io_service::stop, boost::ref(_ioService)));
}

void ASIOLib::Executor::Run(unsigned int numThreads) {
	if (OnRun)
		OnRun(_ioService);

	boost::thread_group workerThreads;
	for (unsigned int i = 0; i < ((numThreads == (unsigned int)-1) ? (boost::thread::hardware_concurrency()) : numThreads); ++i)
		workerThreads.create_thread(boost::bind(&Executor::WorkerThread, this, boost::ref(_ioService)));
	workerThreads.join_all();
}


