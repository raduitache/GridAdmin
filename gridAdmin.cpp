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


#include "ssh_stuff.h"

using namespace std;


vector<int> sd;
vector<string> ip, mac;

void confirmRightChild(string ip, string mac, int sd){
	int sz;
	char *c;
	read(sd, &sz, sizeof(int));
	if(sz < 0)
		cerr << "Couldn't get prefix: ";
	c = new char[sz+1];
	read(sd, c, sizeof(char) * sz);
	c[sz] = 0;
	if(ip.compare(string(c)) != 0){
		c[0] = '0';
		write(sd, c, sizeof(char));
	}
	read(sd, &sz, sizeof(int));
	if(sz < 0)
		cerr << "Couldn't get prefix: ";
	c = new char[sz+1];
	read(sd, c, sizeof(char) * sz);
	c[sz] = 0;
	if(mac.compare(string(c)) != 0){
		c[0] = '0';
		write(sd, c, sizeof(char));
	}
	else{
		c[0] = '1';
		write(sd, c, sizeof(char));
	}
}

void childPlay(string ip, string mac, int sd){
	confirmRightChild(ip, mac, sd);
	ssh_session my_ssh_session = ssh_new();
	if (my_ssh_session == NULL)
		cerr << ip << ":  error at ssh session" << endl;
	ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, ip.c_str());
	if(ssh_connect(my_ssh_session) != SSH_OK){
		cerr << ip << ":   error connecting" << endl;
	}

	if(verify_knownhost(my_ssh_session) < 0){
		ssh_disconnect(my_ssh_session);
	}
	else{
		if(ssh_userauth_password(my_ssh_session, "admin", "adminpass") != SSH_AUTH_SUCCESS){
			cerr << ip << ":   " << ssh_get_error(my_ssh_session) << endl;
			ssh_disconnect(my_ssh_session);
		}
		else{
			cout << ip << ":   is online!" << endl;
		}
	}


	ssh_disconnect(my_ssh_session);
	ssh_free(my_ssh_session);

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

void confirmData(string ip, string mac, int sd){
	int x = ip.size();
	if(write(sd, &x, sizeof(int)) < 0)
		cerr << "Couldn't write... ";
	write(sd, ip.c_str(), sizeof(char) * x);
	x = mac.size();
	write(sd, &x, sizeof(int));
	write(sd, mac.c_str(), sizeof(char) * x);
	char c;
	if(read(sd, &c, sizeof(char)) < 0)
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
	for(int i = 0; i < sd.size(); i++){
		wait(NULL);
	}
}

int main(){
	createChildren();
	parentWork();
}
