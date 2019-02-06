/* See LICENSE for license details. */

#include <sys/stat.h>

bool is_file(char* path) { 
    struct stat sb; stat(path, &sb);
    return S_ISREG(sb.st_mode);
}

void readLine(char* buff, int n) {
    int i=0;
    curs_set(1);
    nocbreak(); echo();
    buff[i] = getch();
    while (i<n-1 && buff[i]!='\n') 
        buff[++i] = getch();
    buff[i] = 0;
    noecho(); cbreak();
    curs_set(0);
}

char* allocAndCopy(const char* s){
    char* ret = (char*) malloc(strlen(s)*sizeof(char));
    strcpy(ret, s);
    return ret;
}