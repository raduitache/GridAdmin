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


using namespace std;


#include "ssh_stuff.h"


vector<int> sd;
vector<string> ip, mac;
int x, y, termline;
string response;
pthread_mutex_t *mx;

vector<int> getMacValuesFromString(string mac){
	vector<int> res(6);
	transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
	for(int i = 0; i < 6; i++){
		if(mac[i * 3] >= 'A')
			res[i] = mac[i * 3] - 'A' + 10;
		else
			res[i] = mac[i * 3] - '0';
		res[i] *= 16;
		if(mac[i * 3 + 1] >= 'A')
			res[i] += mac[i * 3 + 1] - 'A' + 10;
		else
			res[i] += mac[i * 3 + 1] - '0';
	}
	return res;
}

unsigned char* createMagicPacket(vector<int> mac){
	unsigned char *packet = (unsigned char*)malloc(102 * sizeof(unsigned char));
	for(int i = 0; i < 6; i++)
		packet[i] = 0xff;
	for(int i = 1; i <= 16; i++)
		for(int j = 0; j < mac.size(); j++)
			packet[i*6+j] = mac[j];
	return packet;
}

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


void sendMagicPackage(string ip, vector<int> mac){
	unsigned char *packet = createMagicPacket(mac);
	int udpSocket, broadcast = 1;
	struct sockaddr_in server, client;
	bzero(&server, sizeof(server));
	bzero(&client, sizeof(client));

	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	if(setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1){
		response.clear();
		response += "setsockopt (SO_BROADCAST)\n";
		int x = response.length();
		pthread_mutex_lock(mx);
		write(1, &x, sizeof(int));
		write(1, response.c_str(), x * sizeof(char));
		pthread_mutex_unlock(mx);
		exit(EXIT_FAILURE);
	}

	client.sin_family = AF_INET;
	client.sin_addr.s_addr = INADDR_ANY;
	client.sin_port = 0;
	bind(udpSocket, (struct sockaddr*) &client, sizeof(client));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(ip.c_str());
	server.sin_port = htons(9);

	if(sendto(udpSocket, packet, sizeof(unsigned char) * 102, 0, (struct sockaddr*) &server, sizeof(server)) == 102){
		response.clear();
		response += ip;
		response += ":   wake signal sent\n";
		int x = response.length();
		pthread_mutex_lock(mx);
		write(1, &x, sizeof(int));
		write(1, response.c_str(), x * sizeof(char));
		pthread_mutex_unlock(mx);
	}
	close(udpSocket);
	free(packet);
}

void childPlay(string ip, string mac, int sock){
	ssh_session my_ssh_session = ssh_new();
	if (my_ssh_session == NULL){
		response.clear();
		response += ip;
		response += ": error at ssh session\n";
		int x = response.length();
		pthread_mutex_lock(mx);
		write(1, &x, sizeof(int));
		write(1, response.c_str(), x * sizeof(char));
		pthread_mutex_unlock(mx);
	}
	connectSession(my_ssh_session, ip);
	int len;
	char *command;
	while(true){
		read(sock, &len, sizeof(int));
		command = new char[len];
		read(sock, command, len * sizeof(char));
		if(strcmp(command, "stat") == 0){
			if(!ssh_is_connected(my_ssh_session))
				connectSession(my_ssh_session, ip);
			if(ssh_is_connected(my_ssh_session)){
				response.clear();
				response += ip;
				response += ":   online!\n";
				int x = response.length();
				pthread_mutex_lock(mx);
				write(1, &x, sizeof(int));
				write(1, response.c_str(), x * sizeof(char));
				pthread_mutex_unlock(mx);
			}
		}
		else if(strcmp(command, "wake") == 0){
			if(mac.size() == 0){
				response.clear();
				response += ip;
				response += ":   unknown MAC address\n";
				int x = response.length();
				pthread_mutex_lock(mx);
				write(1, &x, sizeof(int));
				write(1, response.c_str(), x * sizeof(char));
				pthread_mutex_unlock(mx);
			}
			else
				sendMagicPackage(ip, getMacValuesFromString(mac));
		}
		else if(strcmp(command, "quit") == 0){
			if(ssh_is_connected(my_ssh_session))
				ssh_disconnect(my_ssh_session);
			ssh_free(my_ssh_session);
			return;
		}
		else {
			if(!ssh_is_connected(my_ssh_session))
				connectSession(my_ssh_session, ip);
			if(ssh_is_connected(my_ssh_session) and sshCommand(my_ssh_session, command) != SSH_OK){
				response.clear();
				response += ip;
				response += ":   Could not execute command: ";
				response += ssh_get_error(my_ssh_session);
				response += '\n';
				int x = response.length();
				pthread_mutex_lock(mx);
				write(1, &x, sizeof(int));
				write(1, response.c_str(), x * sizeof(char));
				pthread_mutex_unlock(mx);
			}
		}
		delete command;
	}
}

string getIp(string c){
	return c.substr(0, c.find(":"));
}

string getMac(string c){
	int x = c.find(":");
	if(x == -1)
		return "";
	return c.substr(x + 1);
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
			response += "Error at fork()\n";
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

void parentWork(){
	string command;
	int len;
	while(command != "quit"){
		getline(cin, command);
		len = command.length() + 1;
		for(int i = 0; i < sd.size(); i++){
			write(sd[i], &len, sizeof(int));
			write(sd[i], command.c_str(), len * sizeof(char));
		}
	}
	for(int i = 0; i < sd.size(); i++)
		wait(NULL);
}

void removeCarriage(char *str){
	int step = 0;
	int len = strlen(str);
	for(int i = 0; i <= len; i++){
		str[i - step] = str[i];
		if(str[i] == '\r')
			step++;
	}
}

void* writeManager(void *args){
    int fd = *(int*) args, len;
	char *command;
    while(true){
		read(fd, &len, sizeof(int));
		command = new char[len+1];
		read(fd, command, len);
		command[len] = 0;
		removeCarriage(command);
		
		if(termline){
			printw("\n");
			termline = 0;
		}
		printw(command);
        refresh();
		delete command;
   }
}

void ncursesDisplay(int sock){
	termline = x = y = 0;
	initscr();
	cbreak();
	noecho();
	scrollok(stdscr, 1);
	keypad(stdscr, TRUE);
	pthread_t thread;
	pthread_create(&thread, 0, writeManager, &sock);
	string command;
	char ch;
	while(command != "quit\n"){
		command.clear();
		while((ch = getch()) != '\n'){
			command += ch;
			if(!termline){
				printw("\n");
				termline = 1;
			}
			printw("%c", ch);
		}
		command.append(1, '\n');
		write(sock, command.c_str(), command.length());	
	}
}

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
