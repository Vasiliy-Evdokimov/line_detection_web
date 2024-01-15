#include <signal.h>
#include <unistd.h>

#include <thread>
#include <pthread.h>

#include "log.hpp"
#include "shared_memory.hpp"
#include "http_controllers.hpp"

#include "service.hpp"

pthread_t p_http_thread;

void* p_http_init(void *args) {
	http_init();
	return 0;
}

void init()
{
	write_log("init()");
	//
	read_config_for_web();
	//
	init_shared_memory();
	//
	write_log("config_sm_id = " + to_string(config_sm_id));
	write_log("config_sm_ptr->PID = " + to_string(config_sm_ptr->PID));
	//
	pthread_create(&p_http_thread, NULL, p_http_init, NULL);
}

void quit()
{
	write_log("quit()");
	//
	//http_quit();
	if (p_http_thread)
		pthread_cancel(p_http_thread);
}

int onLoadConfig()
{
	write_log("onLoadConfig()");
	return 0;
}

int onStart()
{
	write_log("onStart()");
	init();
	return 0;
}

int onRestart()
{
	write_log("onRestart()");
	return 0;
}

void onDestroy()
{
	write_log("onDestroy()");
	quit();
}

int main(int argc, char** argv)
{
	ServiceHandlers srvh {
		onLoadConfig,
		onStart,
		onRestart,
		onDestroy
	};
	//
	LDService srv("line_detection_web", srvh);
	service_main(argc, argv, &srv);
	//
	return 0;
}
