#pragma once

#define _GNU_SOURCE

extern "C" {
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
}

typedef int (*open_t)(const char* path, int flags);
typedef int (*read_t)(int fd, void* data, size_t size);
typedef int (*write_t)(int fd, const void* data, size_t size);

typedef int (*close_t)(int fd);

struct Real
{
    const open_t open;
    const read_t read;
    const write_t write;
    const close_t close;
    Real()
        : open((open_t)dlsym(RTLD_NEXT, "open"))
        , read((read_t)dlsym(RTLD_NEXT, "read"))
        , close((close_t)dlsym(RTLD_NEXT, "close"))
        , write((write_t)dlsym(RTLD_NEXT, "write"))
    {}
};
