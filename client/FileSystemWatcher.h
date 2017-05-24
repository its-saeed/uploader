#ifndef FILE_SYSTEM_WATCHER_H_
#define FILE_SYSTEM_WATCHER_H_

#include <boost/asio.hpp>

class FileSystemWatcher
{
public:
    FileSystemWatcher(boost::asio::io_service&);
    void operator()();
    
private:
    boost::asio::io_service& io_service;
    boost::asio::ip::tcp::socket socket;
};

#endif
