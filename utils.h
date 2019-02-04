#include <sys/stat.h>

bool is_file(char *path){ 
    struct stat sb; stat(path,&sb);
    return S_ISREG(sb.st_mode);
}

void readLine(char *buff,int n){
    int i=0; char c;
    buff[i]=getch();
    while(i<n-1 && buff[i]!='\n') buff[++i]=getch();
    buff[i]=0;
}