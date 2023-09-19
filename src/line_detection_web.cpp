//============================================================================
// Name        : line_detection_web.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "shared_memory.hpp"
#include "http_controllers.hpp"

using namespace std;

bool app_exit = false;

void signalHandler(int aSignal)
{
	cout << "Signal (" << aSignal << ") received.\n";
	//
	http_quit();
	//
	app_exit = true;
}

int main(int argc, char** argv)
{

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGSTOP, signalHandler);

	read_config();
	//
	init_shared_memory();
	//
	cout << "config_sm_id = " << config_sm_id << endl;
	cout << "config_sm_ptr = " << config_sm_ptr << endl;
	cout << "config_sm_ptr->PID = " << config_sm_ptr->PID << endl;

	http_init();

//    //	бесконечный цикл для ожидания сигналов
//    while (!app_exit) {
//    	//
//    }

	return 0;

}
