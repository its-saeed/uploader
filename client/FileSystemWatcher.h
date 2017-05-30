#ifndef FILE_SYSTEM_WATCHER_H_
#define FILE_SYSTEM_WATCHER_H_

#include <boost/asio.hpp>
#include <FileWatcher/FileWatcher.h>
#include <iostream>
#include <boost/asio/deadline_timer.hpp>

class FileSystemWatcher : public FW::FileWatchListener
{
public:
    FileSystemWatcher(boost::asio::io_service&, size_t transmission_unit);
    void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename,
            FW::Action action);
    
private:
    void timer_timeout();
    std::string get_file_part_string(const std::string& file_name, size_t file_id, size_t part_number, size_t part_size, size_t start_byte_index,
            size_t end_byte_index);
    void add_file_to_queue(const std::string path, const std::string& file_name, intmax_t file_size);
    boost::asio::io_service& io_service;
    boost::asio::ip::tcp::socket socket;
    FW::FileWatcher file_watcher;
    boost::asio::deadline_timer timer;
    size_t file_index;
	size_t transmission_unit;
	std::string to_be_sent;
};

#endif
