extern "C" {
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
}
#include <string>

inline void clear(const std::string& dirname)
{
    DIR  *dir = opendir(dirname.c_str());
    char buf[255];

    if (dir) {
        struct dirent *de = readdir(dir);
        while (de) {
            ::sprintf(&buf[0], "%s/%s", dirname.c_str(), de->d_name);
            unlink(&buf[0]);
            de = readdir(dir);
        }
        closedir(dir);
        rmdir(dirname.c_str());
    }
}

inline unsigned long long getns(const struct timespec& tv)
{
    return (unsigned long long)tv.tv_sec * 1000000000 + tv.tv_nsec;
}

inline bool dumpdir(const std::string& dirname)
{
    DIR *dir = opendir(dirname.c_str());

    if (dir) {
        struct dirent *de = readdir(dir);
        while (de) {
    	    #ifdef __linux__
            printf("%lu: %lu %lu %s\n", (size_t)de->d_fileno, (size_t)de->d_reclen, (size_t)de->d_type, de->d_name);
    	    #else
            printf("%d: %d %d %d %s\n", de->d_fileno, de->d_reclen, de->d_type, de->d_namlen, de->d_name);
    	    #endif
            de = readdir(dir);
        }
        closedir(dir);
        return true;
    }
    return false;
}

int main(int argc, char **argv)
{
    printf("LD_PRELOAD:%s\n", getenv("LD_PRELOAD"));
    int         count   = argc > 1 ? atoi(argv[1]) : 100;
    std::string dirname = argc > 2 ? argv[2] : "./tmp";
    const char  *data   = "teteteteteststteststteteststteststteteteststteststteteststtestst";
    char        buf[255];
    const char  *ext[2] = { "txt\0", "lst\0" };

//    clear(dirname);

    mkdir(dirname.c_str(), 0777);
    for (int j = 0; j < count; ++j) {
        ::sprintf(&buf[0], "%s/t%d.txt", dirname.c_str(), j);
        int f = open(&buf[0], O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (write(f, data, strlen(data))) {
        }
        close(f);
    }

    dumpdir(dirname);
    return 0;
}
