#include <signal.h>
#include <unistd.h>

#include "log.hpp"
#include "shared_memory.hpp"
#include "http_controllers.hpp"

void signalHandler(int aSignal)
{

	write_log("Signal (" + to_string(aSignal) + ") received.");
	//
	http_quit();

}

int main(int argc, char** argv)
{

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGSTOP, signalHandler);

	read_config_for_web();
	//
	init_shared_memory();
	//
	write_log("config_sm_id = " + to_string(config_sm_id));
	write_log("config_sm_ptr->PID = " + to_string(config_sm_ptr->PID));

	http_init();

	return 0;

}
