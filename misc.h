#ifndef MISC_H
#define MISC_H

#include <string>
#include <fstream>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>

#include "family.h"

using namespace std;

extern pthread_mutex_t *mx;
extern string response;
extern vector<string> ip, mac;

int setMutex(){
    int fd = shm_open(SHM_FILENAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(fd == -1)
        return -1;
    if(ftruncate(fd, sizeof(pthread_mutex_t)) == -1)
        return -1;
    mx = (pthread_mutex_t *) mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if((long)mx == MAP_SHARED)
        return -1;
    close(fd);
    
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(mx, &mutexattr);
	return 0;
}

void createChildren(){
	int pid;
	string c;
	fstream f("IPs.txt", fstream::in);
	f >> c;
	while(!f.eof()){
		int s[2];
		socketpair(AF_UNIX, SOCK_STREAM, (int)NULL, s);
		sd.push_back(s[0]);
		ip.push_back(getIp(c));
		mac.push_back(getMac(c));
		if((pid = fork()) == -1){
			response.clear();
			response += "\nError at fork()";
			int x = response.length();
			pthread_mutex_lock(mx);
			write(1, &x, sizeof(int));
			write(1, response.c_str(), x * sizeof(char));
			pthread_mutex_unlock(mx);
		}
		if(pid == 0){
			f.close();
			sd.clear();
			ip.clear();
			close(s[0]);
			childPlay(getIp(c), getMac(c), s[1]);
			exit(0);
		}
		close(s[1]);
		f >> c;
	}
	f.close();
}

#endif