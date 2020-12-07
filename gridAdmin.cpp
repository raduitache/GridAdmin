#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <libssh/libssh.h>



using namespace std;


#include "ssh_stuff.h"


vector<int> sd;
vector<string> ip, mac;

void confirmRightChild(string ip, string mac, int sock){
	int sz;
	char *c;
	read(sock, &sz, sizeof(int));
	if(sz < 0)
		cerr << "Couldn't get prefix: ";
	c = new char[sz+1];
	read(sock, c, sizeof(char) * sz);
	c[sz] = 0;
	if(ip.compare(string(c)) != 0){
		c[0] = '0';
		write(sock, c, sizeof(char));
	}
	read(sock, &sz, sizeof(int));
	if(sz < 0)
		cerr << "Couldn't get prefix: ";
	c = new char[sz+1];
	read(sock, c, sizeof(char) * sz);
	c[sz] = 0;
	if(mac.compare(string(c)) != 0){
		c[0] = '0';
		write(sock, c, sizeof(char));
	}
	else{
		c[0] = '1';
		write(sock, c, sizeof(char));
	}
}

void childPlay(string ip, string mac, int sock){
	confirmRightChild(ip, mac, sock);
	ssh_session my_ssh_session = ssh_new();
	if (my_ssh_session == NULL)
		cerr << ip << ":  error at ssh session" << endl;
	connectSession(my_ssh_session, ip);
	int len;
	char *command;
	while(true){
		read(sock, &len, sizeof(int));
		command = new char[len+1];
		read(sock, command, len * sizeof(char));
		command[len] = 0;
		if(strcmp(command, "stat") == 0){
			if(!ssh_is_connected(my_ssh_session))
				connectSession(my_ssh_session, ip);
			if(ssh_is_connected(my_ssh_session))
				cout << ip << ":   online!" << endl;
		}
		if(strcmp(command, "quit") == 0){
			if(ssh_is_connected(my_ssh_session))
				ssh_disconnect(my_ssh_session);
			ssh_free(my_ssh_session);
			return;
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
		if((pid = fork()) == -1)
			cerr << "Error at fork: ";
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

void confirmData(string ip, string mac, int sock){
	int x = ip.size();
	if(write(sock, &x, sizeof(int)) < 0)
		cerr << "Couldn't write... ";
	write(sock, ip.c_str(), sizeof(char) * x);
	x = mac.size();
	write(sock, &x, sizeof(int));
	write(sock, mac.c_str(), sizeof(char) * x);
	char c;
	if(read(sock, &c, sizeof(char)) < 0)
		cerr << "Couldn't read.. ";
	if(c == '0'){
		cerr << "Some matching is wrong.. ";
		return;
	}
}

void parentWork(){
	for(int i = 0; i < sd.size(); i++){
		confirmData(ip[i], mac[i], sd[i]);
	}
	string command;
	int len;
	while(command != "quit"){
		cin >> command;
		len = command.length();
		for(int i = 0; i < sd.size(); i++){
			write(sd[i], &len, sizeof(int));
			write(sd[i], command.c_str(), len * sizeof(char));
		}
	}
	for(int i = 0; i < sd.size(); i++)
		wait(NULL);
}

int main(){
	createChildren();
	parentWork();
}
