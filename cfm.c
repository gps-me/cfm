// This file is part of cfm.

// cfm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// cfm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with cfm.  If not, see <https://www.gnu.org/licenses/>

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include "utils.h"
#include "config.h"

#define MAX_PATH_LENGTH 256
#define MAX_ENTRY 128
#define MAX_BUFFER 256

int MAX_Y, MAX_X, PEG=0, TOTAL;
char CWD[MAX_PATH_LENGTH], PEG_ENTRY[MAX_ENTRY];

/* default sorting scheme - dir first then alphabetic sorting */
int 
defSort(const struct dirent** file1, const struct dirent** file2){
	if((*file1)->d_type==DT_DIR && (*file2)->d_type!=DT_DIR) return -11;
	if((*file1)->d_type!=DT_DIR && (*file2)->d_type==DT_DIR) return 1;
	return strcmp((*file1)->d_name,(*file2)->d_name);
}

/* filter filenames */
int 
filterNames(const struct dirent *file){
	if(file->d_name[0]!='.') return 1;
	return 0;
}

/* prefix entry with other info and marker */
void
prefix(char *dest, struct dirent *file, int i){
	if(i==PEG){ 
		strcpy(PEG_ENTRY,file->d_name);
		strcpy(dest," > ");
	}
	else strcpy(dest,"   ");
	strcat(dest,file->d_name);
}

/* write suffix for entry */
void
suffix(char *dest, struct dirent *file){
	if(file->d_type==DT_DIR) strcat(dest,"/");
}

/* adds a list of files at path at current cursor posn */
void 
ls(char* path){
	struct dirent **files_list;
	char entry[MAX_ENTRY];
	TOTAL=scandir(path, &files_list, filterNames, defSort);
	for(int i=0;i<TOTAL;i++){
		prefix(entry,files_list[i],i);
		suffix(entry,files_list[i]);
		if(files_list[i]->d_type==DT_DIR){
			attron(COLOR_PAIR(1));
			addstr(entry);
			attroff(COLOR_PAIR(1));
		}
		else addstr(entry);
		addch('\n');
	}
}

/* redraws the frame */
void
redraw(){
	clear();
	addstr(CWD);
	move(2,0);
	ls(CWD);
	refresh();
}

/* called on window resize */
void
resizeHandler(int sig){
	getmaxyx(stdscr, MAX_Y, MAX_X);
	redraw();
}

void 
initCurses(){
	initscr(); cbreak(); noecho();
	curs_set(0); // hide cursor
	keypad(stdscr,true);
	getmaxyx(stdscr, MAX_Y, MAX_X);
	signal(SIGWINCH, resizeHandler);
	start_color();
	use_default_colors();
	init_pair(1, DIR_COLOR, -1);
}

bool
handleInput(){
	int n; char buff[MAX_BUFFER],ch;
	switch(getch()){
		// navigation controls
		case KEY_DOWN: PEG=(PEG+1)%TOTAL; return true;
		case KEY_UP: PEG=(PEG==0?TOTAL-1:PEG-1); return true;
		case KEY_ENTER:
		case KEY_RIGHT: // goto highlighted entry or open file
			if(is_file(PEG_ENTRY)){
				char comm[MAX_BUFFER]; 
				sprintf(comm,"xdg-open \"%s\" > /dev/null",PEG_ENTRY);
				system(comm); // TODO run this using fork
			}
			else{
				if(CWD[strlen(CWD)-1]!='/') strcat(CWD,"/");
				strcat(CWD,PEG_ENTRY);
				PEG=0; chdir(CWD);
			}
			return true;
		case KEY_LEFT: // goto parent dir
			n=strlen(CWD)-1;
			while(n>1 && CWD[n]!='/') n--; 
			if(n>=1){ CWD[n]=0; PEG=0; chdir(CWD); }
			return true;
		
		// delete and create
		case 'd':
			sprintf(buff,"sure u want to delete %s (y/n)?",PEG_ENTRY);
			move(MAX_Y-1, 0); addstr(buff); 
			ch=getch(); 
			if(ch=='y' || ch=='Y') remove(PEG_ENTRY);
			return true;
		case 'n':
			curs_set(1); echo();
			move(MAX_Y-1, 0); addstr("name: ");
			readLine(buff,MAX_ENTRY);
			curs_set(0); noecho();
			redraw();
			move(MAX_Y-1, 0);
			addstr("Enter 'f' for file and 'd' for dir");
			ch=getch();
			if(ch=='f'){
				FILE* f=fopen(buff,"w");
				if(f!=NULL) fclose(f);
			}
			else if(ch=='d') mkdir(buff,0777);
			return true;

		// TOOD cut, copy, paste
		
		case 'q': return false;
		default: return true;
	}
}

int
main(){
	initCurses();
	getcwd(CWD, MAX_PATH_LENGTH);
	bool carryOn=true;
	while(carryOn){
		redraw(); carryOn=handleInput();
	}
	endwin();
}