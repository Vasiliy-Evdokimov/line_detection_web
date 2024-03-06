#include <iostream>

#include <thread>
#include <pthread.h>

#include <cstdlib>
#include <cstring>

#include <signal.h>

#include "log.hpp"
#include "config_path.hpp"
#include "shared_memory.hpp"
#include "http_controllers.hpp"

#include "defines.hpp"
#include "service.hpp"

pthread_t p_http_thread;
pthread_t p_check_thread;

#define MAIN_APP_DIR "/usr/bin/line_detection"
#define MAIN_APP_PATH "/usr/bin/line_detection/line_detection"

bool isAppRunning(const char* appName)
{
    char command[100];
    sprintf(command, "pgrep -f \"%s\" -x", appName);
    //
	FILE *cmd = popen(command, "r");
	char pid_str[10];
	fgets(pid_str, sizeof(pid_str), cmd);
	pclose(cmd);
	pid_t pid = atoi(pid_str);
	//
	return (pid != 0) && (kill(pid, 0) == 0);
}

void* p_http_init(void *args)
{
	http_init();
	//
	return 0;
}

void* p_check(void *args)
{
	while (true)
	{
		if (!isAppRunning(MAIN_APP_PATH))
		{
			//	char command[255];
			//	sprintf(command, "cd \"%s\" && exec \"%s\"", MAIN_APP_DIR, MAIN_APP_PATH);
			//	system(command);
			//
			pid_t _pid = fork();
			//
			if (_pid == 0)
			{
				setsid();
				chdir(MAIN_APP_DIR);
				execl(MAIN_APP_PATH, MAIN_APP_PATH, (char*)NULL);
			}
		}
		//
		usleep(1000 * 1000);
	}
	//
	return 0;
}

void init()
{
	write_log(__PRETTY_FUNCTION__);
	//
	read_config_for_web();
	//
	init_shared_memory();
	//
	write_log("config_sm_id = " + to_string(config_sm_id));
	write_log("config_sm_ptr->PID = " + to_string(config_sm_ptr->PID));
	//
	pthread_create(&p_check_thread, NULL, p_check, NULL);
	//
#ifdef SERVICE
	pthread_create(&p_http_thread, NULL, p_http_init, NULL);
#else
	http_init();
#endif
}

void quit()
{
	write_log(__PRETTY_FUNCTION__);
	//
	if (p_http_thread)
		pthread_cancel(p_http_thread);
	if (p_check_thread)
		pthread_cancel(p_check_thread);
}

int onLoadConfig()
{
	write_log(__PRETTY_FUNCTION__);
	return 0;
}

int onStart()
{
	write_log(__PRETTY_FUNCTION__);
	init();
	return 0;
}

int onRestart()
{
	write_log(__PRETTY_FUNCTION__);
	return 0;
}

void onDestroy()
{
	write_log(__PRETTY_FUNCTION__);
	quit();
}

int main(int argc, char** argv)
{
	log_filename =
		get_logs_directory() + "line_detection_web_" +
		GetCurrentTime("%Y-%m-%d-%H-%M-%S") + ".log";
	std::cout << "log_filename = " << log_filename << std::endl;
	//
#ifdef SERVICE
	ServiceHandlers srvh {
		onLoadConfig,
		onStart,
		onRestart,
		onDestroy
	};
	//
	LDService srv("line_detection_web", srvh);
	service_main(argc, argv, &srv);
#else
	init();
#endif
}
