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
#include "send.h"
#include "wol.h"

using namespace std;

extern vector<int> sd;
extern vector<string> ip;
extern pthread_mutex_t *mx;

void connectionWork(string cmd){
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
	string response = "\nConnected to:   ";
	response += ip[i];
	send(response);
	string command;
	int len;
	while(true){
		getline(cin, command);
		len = command.length() + 1;
		if(command.substr(0, 4) == "conn" || command.substr(0, 7) == "connect"){
			string response = "\nYou are already connected";
			send(response);
			continue;
		}
		else if(command == "quit") return;
		write(sd[i], &len, sizeof(int));
		write(sd[i], command.c_str(), len * sizeof(char));
	}
}

void parentWork(){
	string command;
	int len;
	while(command != "quit"){
		getline(cin, command);
		len = command.length() + 1;
		if(command.substr(0, 4) == "conn" || command.substr(0, 7) == "connect"){
			connectionWork(command);
			string response = "\nDisconnected.";
			send(response);
			continue;
		}
		
		for(int i = 0; i < sd.size(); i++){
			write(sd[i], &len, sizeof(int));
			write(sd[i], command.c_str(), len * sizeof(char));
		}
	}
	for(int i = 0; i < sd.size(); i++)
		wait(NULL);

	string response = "\nPress ENTER to return";
	send(response);
	len = -1;
	write(1, &len, sizeof(int));
}

void childPlay(string ip, string mac, int sock){
	ssh_session my_ssh_session = ssh_new();
	if (my_ssh_session == NULL){
		string response;
		response += '\n';
		response += ip;
		response += ": error at ssh session";
		send(response);
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
				string response;
				response += '\n';
				response += ip;
				response += ":   online!";
				send(response);
			}
		}
		else if(strcmp(command, "wake") == 0){
			if(mac.size() == 0){
				string response;
				response += '\n';
				response += ip;
				response += ":   unknown MAC address";
				send(response);
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
				string response;
				response += '\n';
				response += ip;
				response += ":   Could not execute command: ";
				response += ssh_get_error(my_ssh_session);
				send(response);
			}
		}
		delete command;
	}
}

#endif