#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <concurrentqueue.h>
#include <boost/asio.hpp>

#include "UploadWorker.h"
#include "FileSystemWatcher.h"

using namespace std;
using boost::asio::ip::tcp;

moodycamel::ConcurrentQueue<std::string> file_parts_queue;

class UploadManager
{
public:
    UploadManager(boost::asio::io_service& io_service)
    : io_service(io_service)
    , watcher{io_service}
	, worker1{io_service}//, io_service, io_service, io_service, io_service}
	, worker2{io_service}
	, worker3{io_service}
	, worker4{io_service}
	, worker5{io_service}
    {
    }
 
private:
    std::thread file_system_watcher_thread;
	UploadWorker worker1;
	UploadWorker worker2;
	UploadWorker worker3;
	UploadWorker worker4;
	UploadWorker worker5;
	boost::asio::io_service& io_service;
    FileSystemWatcher watcher;
};

int main()
{
    boost::asio::io_service io_service;
    UploadManager manager(io_service);
    io_service.run();
    return 0;
}
