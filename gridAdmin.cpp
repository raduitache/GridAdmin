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

void childPlay(string s){
	cout << s << endl;
}

void createChildren(){
	int pid;
	string c;
	fstream f("IPs.txt", fstream::in);
	f >> c;
	while(!f.eof()){
		int s = socket(AF_UNIX, SOCK_STREAM, NULL);
		sd.push_back(s);
		ip.push_back(c);
		if((pid = fork()) == -1)
			perror("Error at fork: ");
		if(pid == 0){
			f.close();
			sd.clear();
			ip.clear();
			childPlay(c);
			exit(0);
		}
		f >> c;
	}
	f.close();
}

int main(){
	createChildren();
	for(int i = 0; i < sd.size(); i++){
		wait(0);
	}
	printf("\n\nOkay, so now:\n");
	for(int i = 0; i < sd.size(); i++){
		cout << sd[i] << endl << ip[i] << endl;
	}
}
