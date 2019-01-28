#include "misc.h"

#include <thread> //sleep related
#include <chrono> //sleep related
#include <time.h> //clock_gettime
#include <signal.h> //sigaction
#include <string.h> //memcpy, memset
#include <unistd.h> //STDIN_FILE_NO
#include <fcntl.h> //fcntl

uint64_t TimestampUs()
{
	timespec ts;
	if( clock_gettime(CLOCK_MONOTONIC, &ts) )
		DieErrno("Timestamp failed");
	
	return (uint64_t)ts.tv_sec*1000000 + (uint64_t)ts.tv_nsec / 1000;
}

void Sleep(int ms)
{	
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
void SleepUs(int us)
{	
	std::this_thread::sleep_for(std::chrono::microseconds(us));
}


void DieErrno(const char *s)
{
    perror(s);
    exit(1);
}
void Die(const char *s)
{
	fprintf(stderr, s);
	fprintf(stderr, "\n");
	exit(1);
}

void RegisterSignals(void (*signal_handler)(int))
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler=signal_handler;
	if(sigaction(SIGTERM, &action, NULL)==-1)
		DieErrno("sigaction(SIGTERM, &action, NULL)");
	if(sigaction(SIGINT, &action, NULL)==-1)
		DieErrno("sigaction(SIGINT, &action, NULL)");
	if(sigaction(SIGQUIT, &action, NULL)==-1)
		DieErrno("sigaction(SIGQUIT, &action, NULL)");	
	if(sigaction(SIGHUP, &action, NULL)==-1)
		DieErrno("sigaction(SIGHUP, &action, NULL)");	
}

void SetStandardInputNonBlocking()
{
	int flags = fcntl(STDIN_FILENO, F_GETFL);
	if(flags == -1)
		DieErrno("SetStandardInputNonBlocking() fcntl F_GETFL failed");
	
    if( fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1)
		DieErrno("SetStandardInputNonBlocking() fcntl F_GETFL failed");
}

bool IsStandardInputEOF()
{
	char c;
	int status;
	status=read(STDIN_FILENO, &c, 1);
	if(status == -1)
	{	
		if(errno==EAGAIN || errno==EWOULDBLOCK)
			return false;
		else
			DieErrno("IsStandardInputEOF() read failed");	
	}
		
	
	if(status == 0)
		return true;

	Die("Nothing should be on standard input!");
	return true; //make compiler happy
}