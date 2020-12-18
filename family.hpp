#ifndef FAMILY_HPP
#define FAMILY_HPP

#include <vector>
#include <string>
#include <wait.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <libssh/libssh.h>

#include "ssh_stuff.h"
#include "wol.hpp"

using namespace std;

extern vector<int> sd;
extern string response;
extern pthread_mutex_t *mx;

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

void childPlay(string ip, string mac, int sock){
	ssh_session my_ssh_session = ssh_new();
	if (my_ssh_session == NULL){
		response.clear();
		response += '\n';
		response += ip;
		response += ": error at ssh session";
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
				response += '\n';
				response += ip;
				response += ":   online!";
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
				response += '\n';
				response += ip;
				response += ":   unknown MAC address";
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
				response += '\n';
				response += ip;
				response += ":   Could not execute command: ";
				response += ssh_get_error(my_ssh_session);
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

#endif