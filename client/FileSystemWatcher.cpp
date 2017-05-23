#include "FileSystemWatcher.h"
#include <string>

#include <concurrentqueue.h>

extern moodycamel::ConcurrentQueue<std::string> file_parts_queue;

void FileSystemWatcher::operator()()
{
    file_parts_queue.enqueue("output/0/1024");
    file_parts_queue.enqueue("output/1024/2048");
    file_parts_queue.enqueue("output/2048/3072");
    file_parts_queue.enqueue("output/3072/4096");
    file_parts_queue.enqueue("output/4096/5120");
}
