#ifndef SCROLL_HPP
#define SCROLL_HPP

#include <stack>
#include <string>
#include <string.h>
#include <ncurses.h>

using namespace std;

extern stack<string> prevRows, nextRows;

void formatCommand(char *str){
	// get rid of CR, for responses from windows machines.
	int step = 0;
	int len = strlen(str);
	for(int i = 0; i <= len; i++){
		str[i - step] = str[i];
		if(str[i] == '\r')
			step++;
	}


	// we also kinda need to keep track of rows that will be wiped by scroll.
	int newLines = 0;
	len = strlen(str);
	for(int i = 0; i < len; i++)
		newLines += (str[i] == '\n');
	
	// we need to know when and how much we'll be scrolling.
	getyx(stdscr, y, x);
	char smth[257];
	for(int i = 0; i < newLines - (LINES - y - 1); i++){
		mvwinstr(stdscr, i, 0, smth);
		prevRows.push(string(smth));
	}
	move(y, x);
}

void scrollUp(){
	if(!prevRows.empty()){
		char lastLine[COLS + 1];
		getyx(stdscr, y, x);
		mvwinstr(stdscr, LINES - 1, 0, lastLine);
		nextRows.push(string(lastLine));
		scrl(-1);
		mvprintw(0, 0, prevRows.top().c_str());
		prevRows.pop();
		move(y, x);
		curs_set(0);
	}
}

void scrollDown(){
	if(!nextRows.empty()){
		char prevLine[COLS + 1];
		getyx(stdscr, y, x);
		mvwinstr(stdscr, 0, 0, prevLine);
		prevRows.push(string(prevLine));
		scrl(1);
		mvwinstr(stdscr, 0, 0, prevLine);
		mvprintw(LINES-1, 0, nextRows.top().c_str());
		scrl(-1);
		mvprintw(0, 0, prevLine);
		nextRows.pop();
		move(y, x);
		if(nextRows.empty())
			curs_set(1);
	}
}

void resetCursor(){
	while(!nextRows.empty())
		scrollDown();
}

#endif