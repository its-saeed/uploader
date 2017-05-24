#include "FileSystemWatcher.h"
#include <string>

#include <concurrentqueue.h>

extern moodycamel::ConcurrentQueue<std::string> file_parts_queue;

FileSystemWatcher::FileSystemWatcher(boost::asio::io_service& io_service)
: io_service(io_service)
, socket(io_service)
{
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345);
    socket.connect(endpoint);
}

void FileSystemWatcher::operator()()
{
    boost::asio::streambuf stream_buf;
    std::ostream file_info_stream(&stream_buf);
    file_info_stream << uint8_t(0);      // Type of message
    file_info_stream << uint32_t(1);        // File ID
    file_info_stream << uint64_t(5120);     // File size
    file_info_stream << uint16_t(5);        // File parts
    file_info_stream << "output";       // File name
    file_info_stream << "\n";
    boost::asio::write(socket, stream_buf);

    //file_parts_queue.enqueue("output/0/1024");
    //file_parts_queue.enqueue("output/1024/2048");
    //file_parts_queue.enqueue("output/2048/3072");
    //file_parts_queue.enqueue("output/3072/4096");
    //file_parts_queue.enqueue("output/4096/5120");
}
