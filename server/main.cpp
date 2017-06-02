#include <iostream>
#include <cxxopts.hpp>
#include <plog/Log.h>

#include "FileServer.h"
#include "FileMap.h"

using namespace std;

FileMap file_map;

int main(int argc, char** argv)
{
	plog::init(plog::debug, "server_log.txt");
	cxxopts::Options options("File Downloader", "Download files inside a folder.");
	options.add_options()
			("p,port", "Port number of the server.", cxxopts::value<uint16_t>())
			("d,dpath", "Path to place downloaded files.", cxxopts::value<std::string>())
			("t,tranunit","Transmission unit in bytes.", cxxopts::value<size_t>());

	options.parse(argc, argv);
	if (options.count("port") != 1 || options.count("dpath") != 1 || options.count("tranunit") != 1)
	{
		cout << options.help();
		exit(1);
	}

	std::string download_path;
	uint16_t server_port = 0;
	size_t transmission_unit = 1024 * 1024;

	try {
		server_port = options["port"].as<uint16_t>();
		download_path = options["dpath"].as<std::string>();
		transmission_unit = options["tranunit"].as<size_t>();
	} catch (...) {
		cout << options.help();
		exit(1);
	}
	boost::asio::io_service io_service;
	FileServer server(io_service, transmission_unit, server_port);
	file_map.set_download_path(download_path);
    io_service.run();
    return 0;
}
