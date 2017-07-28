#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <concurrentqueue.h>
#include <boost/asio.hpp>

#include <cxxopts.hpp>
#include <plog/Log.h>

#include "UploadWorker.h"
#include "FileSystemWatcher.h"

using namespace std;
using boost::asio::ip::tcp;

moodycamel::ConcurrentQueue<std::string> file_parts_queue;

class UploadManager
{
public:
	UploadManager(boost::asio::io_service& io_service, size_t transmission_unit,
				  std::string server_ip, uint16_t server_port,
				  bool use_proxy, std::string proxy_address, uint16_t proxy_port,
				  uint8_t connections, const std::string& upload_dir)
	: io_service(io_service)
	, watcher(io_service, transmission_unit, server_ip, server_port, use_proxy, proxy_address, proxy_port, upload_dir)
    {
		for (int worker_count = 0; worker_count < connections; ++worker_count)
			workers.push_back(unique_ptr<UploadWorker>(new UploadWorker(io_service, transmission_unit,
																		server_ip, server_port,
																		use_proxy, proxy_address, proxy_port)));
    }
 
private:
	boost::asio::io_service& io_service;
	FileSystemWatcher watcher;
	std::vector<std::unique_ptr<UploadWorker>> workers;
};

int main(int argc, char** argv)
{
	plog::init(plog::debug, "client_log.txt");
	cxxopts::Options options("File Uploader", "Upload files inside a folder to server specified.");
	options.add_options()("i,ip", "IP address of the server.", cxxopts::value<std::string>())
			("p,port", "Port number of the server.", cxxopts::value<uint16_t>())
			("x,proxy", "IP address of the proxy.[optional]", cxxopts::value<string>())
			("o,proxyport", "Port of the proxy.[optional]", cxxopts::value<uint16_t>())
			("c,connections", "Number of simultinous upload workers.", cxxopts::value<uint8_t>())
			("u,upath", "Path to watch for files inside it and upload them.", cxxopts::value<std::string>())
			("t,tranunit","Transmission unit in bytes.", cxxopts::value<size_t>());

	options.parse(argc, argv);

	if (options.count("ip") != 1 || options.count("port") != 1 || options.count("connections") != 1 ||
			options.count("upath") != 1 || options.count("tranunit") != 1)
	{
		cout << options.help();
		exit(1);
	}

	bool use_proxy = options.count("proxy") == 1 ? true : false;
	std::string server_address, upload_path, proxy_address;
	uint16_t server_port = 0, proxy_port = 0;
	uint8_t connections = 1;
	size_t transmission_unit = 1024 * 1024;

	try {
		server_address = options["ip"].as<std::string>();
		server_port = options["port"].as<uint16_t>();
		connections = options["connections"].as<uint8_t>();
		upload_path = options["upath"].as<std::string>();
		transmission_unit = options["tranunit"].as<size_t>();
		if (use_proxy)
		{
			proxy_address = options["proxy"].as<std::string>();
			proxy_port = options["proxyport"].as<uint16_t>();
		}
	} catch (...) {
		cout << options.help();
		exit(1);
	}

	boost::asio::io_service io_service;
	UploadManager manager(io_service, transmission_unit, server_address, server_port, use_proxy, proxy_address, proxy_port, connections, upload_path);
	//UploadManager manager(io_service, 1048576, "192.168.58.128", 12345, true, "127.0.0.1", 808, 4, "D:\\projects\\uploader_exe");

    io_service.run();
    return 0;
}
