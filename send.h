#ifndef SEND_H
#define SEND_H

#include <string>
#include <unistd.h>

using namespace std;

extern pthread_mutex_t *mx;

void send(string response){
    int x = response.length();
	pthread_mutex_lock(mx);
	write(1, &x, sizeof(int));
	write(1, response.c_str(), x * sizeof(char));
	pthread_mutex_unlock(mx);

}

#endif