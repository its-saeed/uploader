#ifndef FILE_INFO_H_
#define FILE_INFO_H_

#include <string>

struct FileInfo
{
    FileInfo()
    : file_id(0)
    , file_size(0)
    , part_no(0)
    {
    }

    FileInfo(size_t file_id, const std::string file_name, intmax_t file_size, size_t part_no)
    : file_id(file_id)
    , file_name(file_name)
    , file_size(file_size)
    , part_no(part_no)
    {
    }

    size_t file_id;
    std::string file_name;
    intmax_t file_size;
    size_t part_no;
};

struct FilePart
{
    FilePart()
    : file_info{0, "", 0, 0}
    , part_number(0)
    , part_size(0)
    , start_byte_index(0)
    , end_byte_index(0)
    , bytes_written(0)
    {
    }

    FileInfo file_info;
    size_t part_number;
    size_t part_size;
    size_t start_byte_index;
    size_t end_byte_index;
    size_t bytes_written;
};

#endif
