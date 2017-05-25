#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <fstream>
#include "FileServer.h"
#include "FileMap.h"

FileMap file_map;

using boost::asio::ip::tcp;
using namespace std;

int main(int argc, char** argv)
{
    boost::asio::io_service io_service;
	FileServer server(io_service);
    io_service.run();
    return 0;
}
