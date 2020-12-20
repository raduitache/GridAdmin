#ifndef IO_HPP
#define IO_HPP

#include <stack>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>

#include "scroll.h"

using namespace std;

extern int termline, x, y;
extern stack<string> prevRows;

void* writeManager(void *args){
    int fd = *(int*) args, len;
	char *command;
    while(true){
		read(fd, &len, sizeof(int));
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
	termline = x = y = 0;
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
	while(command != "quit\n"){
		command.clear();
		while((ch = getch()) != '\n'){
			getyx(stdscr, y, x);
			if(ch == KEY_BACKSPACE){
				mvdelch(y, x-1);
				if(command.length())
					command.erase(x - 1, 1);
			}
			else if(ch == KEY_LEFT){
				move(y, x-1);
			}
			else if(ch == KEY_RIGHT){
				move(y, min(command.length(), (unsigned long)x+1));
			}
			else if (ch == KEY_UP){
				scrollUp();
			}
			else if (ch == KEY_DOWN)
			{
				scrollDown();
			}

			else{
				resetCursor();
				if(!termline){
					printw("\n");
					printw(command.c_str());
					termline = 1;
				}
				getyx(stdscr, y, x);
				command.insert(command.begin() + x, (char)ch);
				if(y == LINES - 1 && ch == '\n'){
					char *line;
					mvwinstr(stdscr, 0, 0, line);
					prevRows.push(string(line));
					move(y, x);
				}
				insch(ch);
				move(y, x+1);
			}
		}
		getyx(stdscr, y, x);
		move(y, command.length());
		command.append(1, '\n');
		write(sock, command.c_str(), command.length());	
	}
}

#endif