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
    , workers{io_service}//, io_service, io_service, io_service, io_service}
    {
    }
 
private:
    std::thread file_system_watcher_thread;
    UploadWorker workers;
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
