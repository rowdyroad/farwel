#include "include/real.h"
#include "include/main.h"

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
    printf("Open Connector(%p)\n", cntr);
    if (!cntr) {
	return main->AddRealFd(real.open(path, flags));
    }
    printf("\t\t%s\n", cntr->Name().c_str());
    int fd = cntr->Open(path, flags);
    main->AddConnector(fd, cntr);
    return fd;
}

int write(int fd, const void* data, size_t size)
{
    Main::Fd* fds = main->GetFd(fd);
    if (!fds) {
	return -1;
    }
    Connector* cntr = fds->GetConnector();
    printf("Write Connector(%p)\n", cntr);
    if (!cntr) {
	return real.write(fds->GetRealFd(), data, size);
    }
    printf("\t\t%s\n", cntr->Name().c_str());
    return cntr->Write(fd, data, size);
};

int close(int fd)
{
    Main::Fd* fds = main->GetFd(fd);
    if (!fds) {
	return -1;
    }
    Connector* cntr = fds->GetConnector();
    printf("Close Connector(%p)\n", cntr);
    if (!cntr) {
	return real.close(fds->GetRealFd());
    }
    printf("\t\t%s\n", cntr->Name().c_str());
    return cntr->Close(fd);
}

int read(int fd, void* data, size_t size)
{
    Main::Fd* fds = main->GetFd(fd);
    if (!fds) {
	return -1;
    }
    Connector* cntr = fds->GetConnector();
    printf("Read Connector(%p)\n", cntr);
    if (!cntr) {
	return real.read(fds->GetRealFd(), data, size);
    }
    return cntr->Read(fd, data, size);
}

int unlink(const char *path)
{
    Connector* cntr = main->GetPathConnector(path);
    printf("Unlink Connector(%p)\n", cntr);
    if (!cntr) {
	return -1;
//	return 
    }
    printf("\t\t%s\n", cntr->Name().c_str());
    return cntr->Unlink(path);
}

}
