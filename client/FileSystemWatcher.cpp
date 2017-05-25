#include "FileSystemWatcher.h"
#include <string>
#include <iostream>
#include <concurrentqueue.h>
#include <boost/bind.hpp>
#include <sys/stat.h>

using namespace std;

extern moodycamel::ConcurrentQueue<std::string> file_parts_queue;

FileSystemWatcher::FileSystemWatcher(boost::asio::io_service& io_service)
: io_service(io_service)
, socket(io_service)
, timer(io_service, boost::posix_time::seconds(1))
, file_index(0)
{
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345);
    socket.connect(endpoint);
    timer.expires_from_now(boost::posix_time::seconds(5));
    timer.async_wait(boost::bind(&FileSystemWatcher::timer_timeout, this));
    FW::WatchID watchID = file_watcher.addWatch("/home/saeed/upload/", this, true);
}

void FileSystemWatcher::operator()()
{
    while (true)
    {
    }

    //file_parts_queue.enqueue("output/0/1024");
    //file_parts_queue.enqueue("output/1024/2048");
    //file_parts_queue.enqueue("output/2048/3072");
    //file_parts_queue.enqueue("output/3072/4096");
    //file_parts_queue.enqueue("output/4096/5120");
}

void FileSystemWatcher::timer_timeout()
{
    file_watcher.update();
    timer.expires_from_now(boost::posix_time::seconds(5));
    timer.async_wait(boost::bind(&FileSystemWatcher::timer_timeout, this));
}

void FileSystemWatcher::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename,
        FW::Action action)
{
    if (action != FW::Actions::Add)
        return;

    struct stat statbuf;
    stat(std::string(dir+filename).c_str(), &statbuf);
    add_file_to_queue(dir, filename, statbuf.st_size);
}

void FileSystemWatcher::add_file_to_queue(const std::string path, const std::string& file_name, intmax_t file_size)
{

    int file_part_count = 0;
    for (file_part_count = 0; file_part_count < file_size / 1024; ++file_part_count)
        file_parts_queue.enqueue(get_file_part_string(path + file_name, file_index, file_part_count, 1024, file_part_count * 1024, (file_part_count + 1) * 1024));

    size_t end_of_file_size = file_size % 1024;

    if (end_of_file_size)
        file_parts_queue.enqueue(get_file_part_string(path + file_name, file_index, file_part_count, end_of_file_size, file_part_count * 1024, (file_part_count * 1024) + end_of_file_size));

    boost::asio::streambuf stream_buf;
    std::ostream file_info_stream(&stream_buf);
    std::string to_be_sent;
    static constexpr char DELIMITER = '|';
    to_be_sent += std::to_string(0) + DELIMITER;        // Type of message
    to_be_sent += std::to_string(file_index) + DELIMITER;        // File ID
    to_be_sent += std::to_string(file_size) + DELIMITER;     // File Size
    to_be_sent += std::to_string(file_part_count) + DELIMITER;        // File Parts
    to_be_sent += file_name + DELIMITER;                 // File name
    to_be_sent += "\n";

    file_info_stream << to_be_sent;
    boost::asio::write(socket, stream_buf);

    ++file_index;
}

std::string FileSystemWatcher::get_file_part_string(const std::string& file_name, size_t file_id, size_t part_number, size_t part_size, size_t start_byte_index,
    size_t end_byte_index)
{
    static constexpr char DELIMITER = '|';
    std::string s = to_string(file_id) + DELIMITER + file_name + DELIMITER + to_string(part_number) + DELIMITER + to_string(part_size) + 
        DELIMITER + to_string(start_byte_index) + DELIMITER + to_string(end_byte_index);

    cout << s << endl;
    return s;
}
