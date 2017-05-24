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
    , file_system_watcher_thread(FileSystemWatcher{io_service})
    , workers{io_service, io_service, io_service, io_service, io_service}
    {
        try{
            auto native_handle = file_system_watcher_thread.native_handle();
            pthread_setname_np(native_handle, "watcher");
            file_system_watcher_thread.detach();
        }
        catch (std::exception& e)
        {
            cout << e.what();
        }
    }
 
private:
    std::thread file_system_watcher_thread;
    UploadWorker workers[5];
    boost::asio::io_service& io_service;
};

int main()
{
    boost::asio::io_service io_service;
    UploadManager manager(io_service);
    io_service.run();
    return 0;
}
