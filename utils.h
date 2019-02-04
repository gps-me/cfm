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