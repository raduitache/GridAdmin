#ifndef IO_HPP
#define IO_HPP

#include <stack>
#include <cmath>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>

#include "scroll.h"

using namespace std;

extern int termline, x, y, terminationFlag;
extern stack<string> prevRows;

int downloadFlag;

void downloadRoutine(int fd){
	uint64_t totalSize, currentSize = 0;
	int currentRead;
	int ticks;
	read(fd, &totalSize, sizeof(uint64_t));
	getyx(stdscr, y, x);
	if(y == LINES - 1);
		scrollDown();
	while(currentSize != totalSize){
		read(fd, &currentRead, sizeof(int));
		if(currentRead == -1){
			pthread_mutex_unlock(mx);
			downloadFlag = 0;
			return;
		}
		currentSize += currentRead;
		ticks = floor((double) currentSize / (double) totalSize * (COLS - 2));
		move(LINES - 1, 0);
		insch('[');
		move(LINES - 1, 1);
		getyx(stdscr, y, x);
		while(x != COLS - 2){
			if(x <= ticks)
				insch('=');
			else
				insch(' ');
			move(LINES-1, ++x);
		}
		insch(']');
		move(LINES-1, ++x);
		refresh();
	}
	downloadFlag = 0;
}

void* writeManager(void *args){
    int fd = *(int*) args, len;
	char *command;
    while(true){
		read(fd, &len, sizeof(int));
		if(len == -1){
			terminationFlag = 1;
			return 0;
		}
		if(len == 0){
			downloadFlag = 1;
			downloadRoutine(fd);
			continue;
		}
		command = new char[len+1];
		read(fd, command, len);
		command[len] = 0;
		resetCursor();
		formatCommand(command);
		
		if(termline){
			termline = 0;
		}
		printw(command);
        refresh();
		delete command;
   }
}

void ncursesDisplay(int sock){
	termline = x = y = downloadFlag = 0;
	initscr();
	cbreak();
	noecho();
	clearok(stdscr, FALSE);
	scrollok(stdscr, TRUE);
	keypad(stdscr, TRUE);
	pthread_t thread;
	pthread_create(&thread, 0, writeManager, &sock);
	string command;
	chtype ch;
	while(!terminationFlag){
		command.clear();
		while((ch = getch()) != '\n'){
			if(downloadFlag)
				continue;
			getyx(stdscr, y, x);
			if (ch == KEY_UP){
				scrollUp();
			}
			else if (ch == KEY_DOWN)
			{
				scrollDown();
			}

			else{
				resetCursor();
				if(!termline){
					if(y == LINES - 1){
						char line[COLS+1];
						mvwinstr(stdscr, 0, 0, line);
						prevRows.push(string(line));
						move(y, x);
					}
					printw("\n");
					printw(command.c_str());
					termline = 1;
				}
				getyx(stdscr, y, x);

				if(ch == KEY_BACKSPACE){
					if(command.length()){
						mvdelch(y, max(x-1, 0));
						command.erase(max(x - 1, 0), 1);
					}
				}
				else if(ch == KEY_LEFT){
					move(y, max(x-1, 0));
				}
				else if(ch == KEY_RIGHT){
					move(y, min(command.length(), (unsigned long)x+1));
				}
				else{
					command.insert(command.begin() + x, (char)ch);
					insch(ch);
					move(y, x+1);
				}
			}
		}
		termline = 0;
		getyx(stdscr, y, x);
		move(y, command.length());
		command.append(1, '\n');
		write(sock, command.c_str(), command.length());	
	}
	pthread_join(thread, NULL);
}

#endif