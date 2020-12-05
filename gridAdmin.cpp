#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;


vector<int> sd;
vector<string> ip;

void confirmRightChild(string ip, int sd){
	int sz;
	read(sd, &sz, sizeof(int));
	if(sz < 0)
		cerr << "Couldn't get prefix: ";
	char c[sz+1];
	read(sd, c, sizeof(char) * sz);
	c[sz] = 0;
	if(ip.compare(string(c)) == 0){
		c[0] = '1';
		write(sd, c, sizeof(char));
	}

}

void childPlay(string s, int sd){
	confirmRightChild(s, sd);

}

void createChildren(){
	int pid;
	string c;
	fstream f("IPs.txt", fstream::in);
	f >> c;
	while(!f.eof()){
		int s[2];
		socketpair(AF_UNIX, SOCK_STREAM, NULL, s);
		sd.push_back(s[0]);
		ip.push_back(c);
		if((pid = fork()) == -1)
			cerr << "Error at fork: ";
		if(pid == 0){
			f.close();
			sd.clear();
			ip.clear();
			close(s[0]);
			childPlay(c, s[1]);
			exit(0);
		}
		close(s[1]);
		f >> c;
	}
	f.close();
}

void confirmRightIP(string ip, int sd){
	int x = ip.size();
	if(write(sd, &x, sizeof(int)) < 0)
		cerr << "Couldn't write... ";
	write(sd, ip.c_str(), sizeof(char) * x);
	char c;
	if(read(sd, &c, sizeof(char)) < 0)
		cerr << "Couldn't read.. ";
	if(c != '1'){
		cerr << "Some matching is wrong.. ";
		return;
	}
	cout << "For ip " << ip << " we have descriptor " << sd << endl;
}

void parentWork(){
	for(int i = 0; i < sd.size(); i++){
		confirmRightIP(ip[i], sd[i]);
	}
}

int main(){
	createChildren();
	parentWork();

}
