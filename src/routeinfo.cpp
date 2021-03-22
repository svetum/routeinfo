#include <iostream>
#include <logger.h>
#include <udp_probe.h>

#include <boost/program_options.hpp>


int main(int argc, char* argv[])
{
	try
	{
		boost::program_options::options_description desc("Options");
		desc.add_options()
			("help", "produce help message")
			("debug", boost::program_options::value<unsigned long>()->default_value(0), "set debug level")
			("destination", boost::program_options::value<std::string>(), "destination")
			("probetype", boost::program_options::value<std::string>(), "probe type");
		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
		boost::program_options::notify(vm);
		
		if(vm.count("help")) 
		{
			std::cout << desc << std::endl;
			return 0;
		}
		if(!vm.count("destination") || !vm.count("probetype")) {
			std::cout << desc << std::endl;
			return 0;
		}
		
		boost::asio::io_context io_context;

		if(vm["probetype"].as<std::string>() == "udp")
		{
			udp_probe* probe = new udp_probe(io_context, vm["destination"].as<std::string>().c_str());
			probe->start();
		}
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}
