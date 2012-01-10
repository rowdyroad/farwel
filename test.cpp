extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
}


int main()
{
    const char* data = "teteteteteststteststteteststteststteteteststteststteteststtestst";
    char buf[255];
    const char* ext[2] = { "txt\0", "lst\0"};
    
    for (int j = 0; j < 1000; ++j ) {
	::sprintf(&buf[0], "./tmp/t%d.txt", j);
        int f = open(&buf[0], O_CREAT | O_WRONLY | O_TRUNC, 0666);
	write(f, data, strlen(data));
        close(f);
    }
    return 0;
}
