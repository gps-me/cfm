/* See LICENSE for license details. */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include "utils.h"
#include "config.h"

#define MAX_PATH_LENGTH 512
#define MAX_ENTRY 256
#define MAX_BUFFER 512

typedef enum action { CUT, COPY } action;
struct task { char *path; char *destName; action act;  } task;

static int MAX_Y, MAX_X, PEG=0, TOTAL_ENTRY=0, TOTAL_TASKS=0;
static bool SHOW_HIDDEN = false;
static char CWD[MAX_PATH_LENGTH], PEG_ENTRY_NAME[MAX_ENTRY];
static struct task* TASK_QUEUE = NULL;

static void prefix(char*, const struct dirent*, const int);
static void suffix(char*, const struct dirent*);
static int defSort(const struct dirent**, const struct dirent**);
static int filterNames(const struct dirent*);
static void ls(const char*);
static void redraw();
static void resizeHandler(int);
static void initCurses();
static void addTask(const char*, const char*, action);
static void fCopy(const char*, const char*);
static void fMove(const char*, const char*);
static void performTasks();
static bool handleInput();

/* prefix entry with other info and marker */
void
prefix(char* dest, const struct dirent* file, const int i) {
	if (i==PEG) { 
		strcpy(PEG_ENTRY_NAME, file->d_name);
		strcpy(dest, " > ");
	} else { 
		strcpy(dest, "   ");
	}
	strcat(dest, file->d_name);
}

/* write suffix for entry */
void
suffix(char* dest, const struct dirent* file) {
	if (file->d_type==DT_DIR) {
		strcat(dest, "/");
	}
}

/* default sorting scheme - dir first then alphabetic sorting */
int
defSort(const struct dirent** file1, const struct dirent** file2) {
	if ((*file1)->d_type==DT_DIR && (*file2)->d_type!=DT_DIR) { return -1; }
	if ((*file1)->d_type!=DT_DIR && (*file2)->d_type==DT_DIR) { return  1; }
	return strcmp((*file1)->d_name, (*file2)->d_name);
}

/* filter filenames */
int
filterNames(const struct dirent* file) {
	if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, "..")) return 0;
	if (SHOW_HIDDEN || file->d_name[0]!='.') return 1;
	return 0;
}

/* adds a list of files at path at current cursor posn */
void
ls(const char* path) {
	struct dirent** files_list;
	char entry[MAX_ENTRY];
	TOTAL_ENTRY = scandir(path, &files_list, filterNames, defSort);
	for(int i=0; i<TOTAL_ENTRY; i++) {
		prefix(entry, files_list[i], i);
		suffix(entry, files_list[i]);
		if (files_list[i]->d_type==DT_DIR) {
			attron(COLOR_PAIR(1));
			addstr(entry);
			attroff(COLOR_PAIR(1));
		} else { 
			addstr(entry); 
		}
		addch('\n');
		free(files_list[i]);
	}
	free(files_list);
}

/* redraws the frame */
void
redraw() {
	erase();
	move(0, 0); addstr(CWD);
	move(2, 0); ls(CWD);
	refresh();
}

/* called on window resize */
void
resizeHandler(int sig) {
	getmaxyx(stdscr, MAX_Y, MAX_X);
	redraw();
}

/* initialise curses window */
void
initCurses() {
	initscr(); cbreak(); noecho();
	curs_set(0); // hide cursor
	keypad(stdscr, true);
	getmaxyx(stdscr, MAX_Y, MAX_X);
	signal(SIGWINCH, resizeHandler);
	start_color();
	use_default_colors();
	init_pair(1, DIR_COLOR, -1);
}

/* creates and adds a new task to the task buffer */
void
addTask(const char* path, const char* destName, action act) {
	if (TASK_QUEUE==NULL) TOTAL_TASKS = 0;
	TASK_QUEUE = realloc(TASK_QUEUE, (TOTAL_TASKS+1)*sizeof(struct task));
	if (TASK_QUEUE==NULL) {
		fprintf(stderr, "Out of memory!!\n");
		exit(errno);
	}
	TASK_QUEUE[TOTAL_TASKS].path = allocAndCopy(path);
	TASK_QUEUE[TOTAL_TASKS].destName = allocAndCopy(destName);
	TASK_QUEUE[TOTAL_TASKS].act = act;
	TOTAL_TASKS++;
}

/* copy file or dir */
void
fCopy(const char* from, const char* to) {
	char comm[MAX_BUFFER];
	strcpy(comm, "cp -fr \"");
	strcat(comm, from); strcat(comm, "\" \"");
	strcat(comm, to); strcat(comm, "\"");
	system(comm);
}

/* move file or dir */
void
fMove(const char* from, const char* to) {
	char comm[MAX_BUFFER];
	strcpy(comm, "mv -f \"");
	strcat(comm, from); strcat(comm, "\" \"");
	strcat(comm, to); strcat(comm, "\"");
	system(comm);
}

/* perform all tasks in task buffer and empty it */
void
performTasks() {
	int i=-1;
	while (++i<TOTAL_TASKS) {
		switch (TASK_QUEUE[i].act) {
			case COPY: fCopy(TASK_QUEUE[i].path, TASK_QUEUE[i].destName); break;
			case CUT: fMove(TASK_QUEUE[i].path, TASK_QUEUE[i].destName); break;
			default: break;
		}
		free(TASK_QUEUE[i].path);
		free(TASK_QUEUE[i].destName);
	}
	free(TASK_QUEUE); 
	TOTAL_TASKS = 0;
}

/* handle all key inputs */
bool
handleInput() {
	int n, c=getch(); char buff[MAX_BUFFER];
	char task_path[MAX_BUFFER], task_dest_name[MAX_BUFFER];
	switch(c) {

		/* navigation controls */
		case KEY_DOWN: PEG=(PEG+1)%TOTAL_ENTRY; break;
		case KEY_UP: PEG=(PEG==0?TOTAL_ENTRY-1:PEG-1); break;
		
		case KEY_ENTER: case KEY_RIGHT: /* open or goto pegged entry */
			if (is_file(PEG_ENTRY_NAME)) {
				char comm[MAX_BUFFER]; 
				sprintf(comm, "xdg-open \"%s\" > /dev/null", PEG_ENTRY_NAME);
				system(comm); // TODO run this using fork
			}
			else if (TOTAL_ENTRY>0) {
				if (CWD[strlen(CWD)-1]!='/') strcat(CWD, "/");
				strcat(CWD, PEG_ENTRY_NAME);
				PEG=0; chdir(CWD);
			}
			break;

		case KEY_LEFT: /* goto parent directory */
			n = strlen(CWD)-1;
			while(n>1 && CWD[n]!='/') n--; 
			if (n>=1) { CWD[n]=PEG=0; chdir(CWD); }
			break;
		
		case 'd': /* delete */
			sprintf(buff, "Delete %s (y/n)?", PEG_ENTRY_NAME);
			move(MAX_Y-1, 0); addstr(buff); 
			c = getch();
			if (c=='y' || c=='Y') { remove(PEG_ENTRY_NAME); }
			break;
		
		case 'n': /* create file or dir */
			move(MAX_Y-1, 0); addstr("name: ");
			readLine(buff, MAX_BUFFER);
			redraw();
			move(MAX_Y-1, 0); addstr("Enter 'f' for file and 'd' for dir");
			c = getch();
			if (c=='f') {
				FILE* f = fopen(buff, "w");
				if (f!=NULL) fclose(f);
			} else if (c=='d') {
				mkdir(buff, 0777);
			}
			break;

		case 'x': case 'c': /* cut or copy */
			strcpy(task_path, CWD); strcat(task_path, "/"); strcat(task_path, PEG_ENTRY_NAME);
			strcpy(task_dest_name, PEG_ENTRY_NAME);
			addTask(task_path, task_dest_name, (c=='x'?CUT:COPY));
			break;
		
		case 'v': case 'p': /* paste */
			performTasks();
			break;

		case 'r': /* rename */
			move(MAX_Y-1, 0); addstr("Enter new name: ");
			readLine(buff, MAX_BUFFER);
			fMove(PEG_ENTRY_NAME, buff);
			break;

		case '.': SHOW_HIDDEN=!SHOW_HIDDEN; break;
		case 'q': return false;
	}
	return true;
}

int
main(){
	initCurses();
	getcwd(CWD, MAX_PATH_LENGTH);
	bool carryOn = true;
	while (carryOn) {
		redraw();
		carryOn = handleInput();
	}
	endwin();
}