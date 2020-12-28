#ifndef CONN_H
#define CONN_H

#include <string>
#include <vector>
#include <iostream>
#include <libssh/libssh.h>

#include "send.h"

using namespace std;


void parentConn(string cmd){
	int i;
	int p = cmd.rfind(' ');
	for(i = 0; i < ip.size(); i++)
		if(ip[i] == cmd.substr(p + 1))
			break;

	if(i == ip.size()){
		string response = "\nThis ip is not valid";
		send(response);
		return;
	}
	string command;
	int len;
    len = 5;
	write(sd[i], &len, sizeof(int));
	write(sd[i], cmd.substr(0, 4).c_str(), len * sizeof(char));
	read(sd[i], &len, sizeof(int));
	if(len == -1){
		string response = "\nCannot continue";
		send(response);
		return;
	}
	string response = "\nWe connected";
	send(response);
}

void childConn(string ip, string mac, int sock){
	ssh_session session = ssh_new();
	int len;
	if(session == NULL){
		len = -1;
		write(sock, &len, sizeof(int));
		return;
	}
	len = 0;
	write(sock, &len, sizeof(int));	
}

#endif