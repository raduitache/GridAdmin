#ifndef WOL_HPP
#define WOL_HPP

#include <vector>
#include <string>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <arpa/inet.h>

using namespace std;

extern pthread_mutex_t *mx;

unsigned char* createMagicPacket(vector<unsigned char> mac){
	unsigned char *packet = (unsigned char*)malloc(102 * sizeof(unsigned char));
	for(int i = 0; i < 6; i++)
		packet[i] = 0xff;
	for(int i = 1; i <= 16; i++)
		for(int j = 0; j < mac.size(); j++)
			packet[i*6+j] = mac[j];
	return packet;
}

void sendMagicPackage(string ip, vector<unsigned char> mac){
	unsigned char *packet = createMagicPacket(mac);
	int udpSocket, broadcast = 1;
	struct sockaddr_in server, client;
	bzero(&server, sizeof(server));
	bzero(&client, sizeof(client));

	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	if(setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1){
		string response;
		response += "\nsetsockopt (SO_BROADCAST)";
		send(response);
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
		string response;
		response += '\n';
		response += ip;
		response += ":   wake signal sent";
		send(response);
	}
	close(udpSocket);
	free(packet);
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

vector<unsigned char> getMacValuesFromString(string mac){
	vector<unsigned char> res(6);
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

#endif