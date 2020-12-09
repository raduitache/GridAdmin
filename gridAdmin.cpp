#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
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


void sendMagicPackage(string ip, vector<int> mac){
	unsigned char *packet = createMagicPacket(mac);
	int udpSocket, broadcast = 1;
	struct sockaddr_in server, client;
	bzero(&server, sizeof(server));
	bzero(&client, sizeof(client));

	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	if(setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1){
		perror("setsockopt (SO_BROADCAST)");
		exit(EXIT_FAILURE);
	}

	client.sin_family = AF_INET;
	client.sin_addr.s_addr = INADDR_ANY;
	client.sin_port = 0;
	bind(udpSocket, (struct sockaddr*) &client, sizeof(client));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("192.168.1.3");
	server.sin_port = htons(9);

	if(sendto(udpSocket, packet, sizeof(unsigned char) * 102, 0, (struct sockaddr*) &server, sizeof(server)) == 102)
		cout << ip << ":   wake signal sent" << endl;
	close(udpSocket);
	free(packet);
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
		else if(strcmp(command, "wake") == 0){
			if(mac.size() == 0)
				cout << ip << ":   unknown MAC address" << endl;
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
			if(ssh_is_connected(my_ssh_session) and sshCommand(my_ssh_session, command) != SSH_OK)
				cout << ip << ":   Could not execute command: " << ssh_get_error(my_ssh_session) << endl;
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
		getline(cin, command);
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
