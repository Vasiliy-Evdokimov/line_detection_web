#include <iostream>

#include "log.hpp"
#include "config_path.hpp"
#include "shared_memory.hpp"
#include "http_controllers.hpp"

int main(int argc, char** argv)
{
	read_config_for_web();
	//
	init_shared_memory();
	//
	http_init();
	//
	return 0;
}
