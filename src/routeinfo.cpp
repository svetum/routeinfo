#include <iostream>
#include <logger.h>
#include <icmp_probe.h>
#include <icmp_tx.h>
#include <udp_probe.h>
#include <udp_tx.h>

#include <boost/program_options.hpp>


int main(int argc, char* argv[])
{
	try
	{
		boost::program_options::options_description desc("Options");
		desc.add_options()
			("help", "produce help message")
			("probetype", boost::program_options::value<std::string>()->default_value(""), "probe type")
			("tx", boost::program_options::value<std::string>()->default_value(""), "tx type")
			("debug", boost::program_options::value<unsigned long>()->default_value(0), "set debug level")
			("destination", boost::program_options::value<std::string>()->default_value(""), "destination")
			("port", boost::program_options::value<uint16_t>()->default_value(0), "destination port")
			("hops", boost::program_options::value<uint8_t>()->default_value(0), "number of hops till destionation")
			("packets", boost::program_options::value<uint32_t>()->default_value(0), "number of packets to transmit")
			("interval", boost::program_options::value<uint32_t>()->default_value(0), "interval between the packets")
			("payload", boost::program_options::value<uint16_t>()->default_value(0), "payload size");
			
		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
		boost::program_options::notify(vm);
		
		if(vm.count("help")) 
		{
			std::cout << desc << std::endl;
			return 0;
		}
		
		boost::asio::io_context io_context;

		if(vm["probetype"].as<std::string>() == "udp")
		{
			udp_probe* probe = new udp_probe(io_context, vm["destination"].as<std::string>().c_str());
			probe->start();
		} else if(vm["probetype"].as<std::string>() == "icmp")
		{
			icmp_probe* probe = new icmp_probe(io_context, vm["destination"].as<std::string>().c_str());
			probe->start();
		}

		if(vm.count("tx") && vm.count("destination") && vm.count("port") && vm.count("hops") && vm.count("packets") && vm.count("interval") && vm.count("payload") && vm["tx"].as<std::string>() == "udp") 
		{
			udp_tx* tx = new udp_tx(io_context, vm["destination"].as<std::string>().c_str(), vm["port"].as<uint16_t>(), vm["hops"].as<uint8_t>(), vm["packets"].as<uint32_t>(), vm["interval"].as<uint32_t>(), vm["payload"].as<uint16_t>());
			tx->start();
		} 
		if(vm.count("tx") && vm.count("destination") && vm.count("port") && vm.count("hops") && vm.count("packets") && vm.count("interval") && vm.count("payload") && vm["tx"].as<std::string>() == "icmp") 
		{
			std::cout << "Strating icmp_tx" << std::endl;
			icmp_tx* tx = new icmp_tx(io_context, vm["destination"].as<std::string>().c_str(), vm["hops"].as<uint8_t>(), vm["packets"].as<uint32_t>(), vm["interval"].as<uint32_t>(), vm["payload"].as<uint16_t>());
			tx->start();
		}
		
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}
