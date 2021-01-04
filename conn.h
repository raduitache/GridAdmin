#ifndef CONN_H
#define CONN_H

#include <string>
#include <vector>
#include <iostream>
#include <libssh/libssh.h>

#include "send.h"
#include "sftp.h"

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
	while(true){
		getline(cin, command);
		len = command.length() + 1;
		if(command.substr(0, 4) == "conn" || command.substr(0, 7) == "connect"){
			string response = "\nYou are already connected";
			send(response);
			continue;
		}
		write(sd[i], &len, sizeof(int));
		write(sd[i], command.c_str(), len * sizeof(char));
		read(sd[i], &len, sizeof(int));
		if(len == -1){
			string response = "\n";
			response += ip[i];
			response += ":   Disconnected.";
			send(response);
			return;
		}
	}
}

void childConn(string ip, string mac, int sock){
	ssh_session session = ssh_new();
	int len;
	if(session == NULL){
		len = -1;
		write(sock, &len, sizeof(int));
		return;
	}
	connectSession(session, ip);
	if(!ssh_is_connected(session)){
		len = -1;
		write(sock, &len, sizeof(int));
		return;
	}
	len = 0;
	char * command;
	write(sock, &len, sizeof(int));	
	while(true){
		read(sock, &len, sizeof(int));
		command = new char[len];
		read(sock, command, len * sizeof(char));
		if(strcmp(command, "quit") == 0){
			len = -1;
			write(sock, &len, sizeof(int));
			return;
		}
		if(string(command).substr(0, 8) == "download"){
			sftp_session sftp;
			sftp = sftp_new(session);
			sftp_init(sftp);
			string params[2];
			getDownloadParams(command, params);
			if(string(command) != params[0])
				sftp_read_sync(session, sftp, params[0], params[1]);
			sftp_free(sftp);
			len = 0;
			write(sock, &len, sizeof(int));
		}
		else {
			if(sshCommand(session, command) != SSH_OK){
				string response;
				response += '\n';
				response += ip;
				response += ":   Could not execute command: ";
				response += ssh_get_error(session);
				send(response);
			}
			len = 0;
			write(sock, &len, sizeof(int));
		}
		delete command;
	}

}

#endif