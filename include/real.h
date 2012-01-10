#pragma once

#define _GNU_SOURCE

extern "C" {
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
}


typedef int (*open_t)(const char* path, int flags);
typedef int (*read_t)(int fd, void* data, size_t size);
typedef int (*write_t)(int fd, const void* data, size_t size);
typedef int (*close_t)(int fd);
typedef int (*namedmode_t)(const char* name, mode_t);
typedef int (*named_t)(const char* name);
typedef DIR* (*opendir_t)(const char* name);
typedef struct dirent* (*readdir_t)(DIR* dd);
typedef int (*closedir_t)(DIR* dd);

struct Real
{
    const open_t open;
    const read_t read;
    const write_t write;
    const close_t close;
    const named_t unlink;
    const namedmode_t mkdir;
    const named_t rmdir;
    const opendir_t opendir;
    const readdir_t readdir;
    const closedir_t closedir;
    
    Real()
        : open((open_t)dlsym(RTLD_NEXT, "open"))
        , read((read_t)dlsym(RTLD_NEXT, "read"))
        , close((close_t)dlsym(RTLD_NEXT, "close"))
        , write((write_t)dlsym(RTLD_NEXT, "write"))
        , unlink((named_t)dlsym(RTLD_NEXT, "unlink"))
        , mkdir((namedmode_t)dlsym(RTLD_NEXT, "mkdir"))
        , rmdir((named_t)dlsym(RTLD_NEXT, "rmdir"))
        , opendir((opendir_t)dlsym(RTLD_NEXT, "opendir"))
        , readdir((readdir_t)dlsym(RTLD_NEXT, "readdir"))
        , closedir((closedir_t)dlsym(RTLD_NEXT, "closedir"))
    {}
    
};
