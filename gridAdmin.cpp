#include <stack>
#include <vector>
#include <string>
#include <fstream>
#include <fcntl.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <ncurses.h>
#include <algorithm>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <libssh/libssh.h>

#define SHM_FILENAME "gridAdmin.shm"
#include "ssh_stuff.h"

using namespace std;

vector<int> sd;
vector<string> ip, mac;
stack<string> prevRows, nextRows;
int x, y, termline;
string response;
pthread_mutex_t *mx;

#include "io.hpp"
#include "misc.h"

int main(){
	int sp[2];
	socketpair(AF_UNIX, SOCK_DGRAM, 0, sp);
	int pid = fork();
	if(pid == -1){
		cerr << "Error at fork()" << endl;
		return -1;
	}
	if(pid == 0){
		dup2(sp[1], 0);
		dup2(sp[1], 1);
		dup2(sp[1], 2);
		close(sp[0]);
		close(sp[1]);
		// since it's only these guys that need to synch their prints:
		setMutex();
		createChildren();
		parentWork();
		munmap(mx, sizeof(pthread_mutex_t));
		shm_unlink(SHM_FILENAME);
		return 0;
	}
	close(sp[1]);
	ncursesDisplay(sp[0]);
	wait(0);
	endwin();
}
