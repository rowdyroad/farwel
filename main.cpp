#include "include/real.h"
#include "include/main.h"

#include <assert.h>
#include "include/debug.h"
static Real real;
static std::auto_ptr<Main> main;

extern "C" {
void __attribute__((constructor)) init(void) 
{
    const char* config_file =  getenv("FRWL_CONFIG_FILE");
    if (!config_file) {
	fprintf(stderr, "Config file is not set. Set env param FRWL_CONFIG_FILE\n");
	exit(-1);
    }
    printf("NANO: init() %s\n", config_file);
    main.reset(new Main(real, config_file));
}

int open(const char *path, int flags, ...)
{
    Connector* cntr = main->GetPathConnector(path);
    assert(cntr);
    printf("Open Connector(%p): %s\n", cntr, cntr->Name().c_str());
    return cntr->Open(path, flags);
}

int creat(const char* path, mode_t mode)
{
    return open(path, O_CREAT | O_TRUNC | O_WRONLY, mode);
}

int write(int fd, const void* data, size_t size)
{
    Connector* cntr = main->GetConnector(fd);
    printf("Write Connector(%p): ", cntr);
    if (!cntr) {
	//! todo real write or something...think
	printf("undefined\n");
	return -1;
    }
    printf("%s\n", cntr->Name().c_str());
    return cntr->Write(fd, data, size);
};

int close(int fd)
{
    Connector* cntr = main->GetConnector(fd);
    printf("Write Connector(%p)\n", cntr);
    if (!cntr) {
	//! todo real write or something...think
	printf("undefined\n");
	return -1;
    }
    printf("%s\n", cntr->Name().c_str());
    return cntr->Close(fd);
}

int read(int fd, void* data, size_t size)
{
    Connector* cntr = main->GetConnector(fd);
    printf("Read Connector(%p)\n", cntr);
    if (!cntr) {
	//! todo real write or something...think
	printf("undefined\n");
	return -1;
    }
    printf("%s\n", cntr->Name().c_str());
    return cntr->Read(fd, data, size);
}

/*
int stat(const char* file, struct stat* buf)
{
    printf("Stat %s\n", file);
    return -1;
}

int fstat(int fd, struct stat *buf)
{
    printf("Stat %d\n", fd);
    return -1;
}

int lstat(const char *file, struct stat *buf)
{
    printf("Lstat %s\n", file);
    return -1;
}

int link(const char* file, const char* link)
{
    printf("link %s -> %s\n", file, link);
    return -1;
}

int symlink(const char* file, const char* link)
{
    printf("symlink %s -> %s\n", file, link);
    return -1;
}

int rename(const char* name, const char* newname)
{
    printf("rename %s -> %s\n", name, newname);
    return -1;
}*/

int rmdir(const char* dir)
{
    printf("rmdir: %s\n", dir);
    
    return 0;
}

int mkdir(const char* dir, mode_t mode)
{
    printf("mkdir: %s\n", dir);
    
    return 0;
}

DIR* opendir(const char* dir)
{
    Connector* cntr = main->GetPathConnector(dir);
    printf("OpenDir(%s) Connector(%p)\n",dir, cntr);
    if (!cntr) {
	//! todo real write or something...think
	printf("undefined\n");
	return NULL;
    }
    printf("%s\n", cntr->Name().c_str());
    return cntr->OpenDir(dir);
}

struct dirent * readdir(DIR* dd)
{
    printf("readdir: %p\n", dd);
    return NULL;

}

int closedir(DIR* dir)
{
    Connector* cntr = main->GetConnector(dir);
    printf("CloseDir Connector(%p)\n", cntr);
    if (!cntr) {
	//! todo real write or something...think
	printf("undefined\n");
	return -1;
    }
    printf("%s\n", cntr->Name().c_str());
    return cntr->CloseDir(dir);
}

int unlink(const char *path)
{
    Connector* cntr = main->GetPathConnector(path);
    assert(cntr);
    printf("Unlink Connector(%p) %s\n", cntr, cntr->Name().c_str());
    return cntr->Unlink(path);
}

}
