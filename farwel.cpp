extern  "C" {
#include <assert.h>
#include <stdarg.h>
#include <math.h>
}
#include "include/real.h"
#include "include/main.h"
#include "include/path.h"

static FWL::Real                real;
static std::auto_ptr<FWL::Main> main;
static FWL::Path                path_master;
static uid_t uid = ::getuid();
static uid_t gid = ::getgid();

#define VA_ARG(type, var, arg)     \
    va_list list;                  \
    va_start(list, arg);           \
    type var = va_arg(list, type); \
    va_end(list);

#ifndef NDEBUG
typedef boost::unordered_map<int, std::string>   Files;
static Files      files;
static const char *not_found = "not found";
const char *File(int id)
{
    Files::const_iterator it = files.find(id);

    if (it == files.end()) {
        return not_found;
    }
    return it->second.c_str();
}

#else
static const char *indebug_mode = "(filename shown in debug mode only)";
const char *File(int id)
{
    return indebug_mode;
}

#endif


extern "C" {
    void __attribute__((constructor)) init(void)
    {
        const char *config_file = getenv("FRWL_CONFIG_FILE");

        if (!config_file) {
            fprintf(stderr, "Config file is not set. Set env param FRWL_CONFIG_FILE\n");
            exit(-1);
        }
        main.reset(new FWL::Main(config_file));
    }

    int open(const char *path, int flags, ...)
    {
        if (!main.get()) {
            VA_ARG(int, mode, flags);
#ifndef NDEBUG
            int i = (mode) ? real.open(path, flags, mode) : real.open(path, flags);
            if (i != -1) {
                files.insert(std::make_pair(i, path));
            }
            return i;
#else
            return (mode) ? real.open(path, flags, mode) : real.open(path, flags);
#endif
        }
        std::string    realpath = path_master.Absolute(path);
        FWL::Connector *cntr    = main->GetConnector(realpath);
        main->Logger().Inf("Open FWL::Connector(%p): %s", cntr, realpath.c_str());
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            VA_ARG(int, mode, flags);
#ifndef NDEBUG
            int i = (mode) ? real.open(path, flags, mode) : real.open(path, flags);
            if (i != -1) {
                files.insert(std::make_pair(i, path));
            }
            return i;
#else
            return (mode) ? real.open(path, flags, mode) : real.open(path, flags);
#endif
        }
        main->Logger().Inf("%s", cntr->Name().c_str());
        return cntr->Open(realpath, flags);
    }

    int creat(const char *path, mode_t mode)
    {
        return open(path, O_CREAT | O_TRUNC | O_WRONLY, mode);
    }

    ssize_t write(int fd, const void *data, size_t size)
    {
        if (!main.get()) {
            return real.write(fd, data, size);
        }
        FWL::Connector *cntr = main->GetConnector(fd);
        main->Logger().Inf("Write FWL::Connector(%p): ", cntr);
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.write(fd, data, size);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->Write(fd, data, size);
    }

    int close(int fd)
    {
        if (!main.get()) {
            return real.close(fd);
        }
        FWL::Connector *cntr = main->GetConnector(fd);
        main->Logger().Inf("Write FWL::Connector(%p) %d - %s: ", cntr, fd, File(fd));
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.close(fd);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->Close(fd);
    }

    ssize_t read(int fd, void *data, size_t size)
    {
        if (!main.get()) {
            return real.read(fd, data, size);
        }
        FWL::Connector *cntr = main->GetConnector(fd);
        main->Logger().Inf("Read FWL::Connector(%p) %d - %s: ", cntr, fd, File(fd));
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.read(fd, data, size);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->Read(fd, data, size);
    }

    void __setStat(FWL::Connector *cntr, struct stat *buf, size_t size)
    {
        ::memset(buf, 0, sizeof(struct stat));
        buf->st_mode    = S_IFREG | S_IFDIR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        buf->st_uid     = uid;
        buf->st_gid     = gid;
        buf->st_blksize = cntr->GetBlockSize();
        buf->st_size    = size;
        buf->st_blocks  = (blkcnt_t) ::ceil((double)size / buf->st_blksize);
    }

    int stat(const char *path, struct stat *buf)
    {
        if (!main.get()) {
            return real.stat(path, buf);
        }
        std::string    realpath = path_master.Absolute(path);
        FWL::Connector *cntr    = main->GetConnector(realpath);
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.stat(path, buf);
        }

        size_t size = 0;
        if (!cntr->GetFileSize(realpath, size)) {
            errno = ENOENT;
            return -1;
        }
        __setStat(cntr, buf, size);
        return 0;
    }

    int fstat(int fd, struct stat *buf)
    {
        if (!main.get()) {
            return real.fstat(fd, buf);
        }
        FWL::Connector *cntr = main->GetConnector(fd);
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.fstat(fd, buf);
        }
        size_t size = 0;
        if (!cntr->GetFileSize(fd, size)) {
            errno = ENOENT;
            return -1;
        }
        __setStat(cntr, buf, size);
        return 0;
    }

    int lstat(const char *file, struct stat *buf)
    {
        return stat(file, buf);
    }

    int rmdir(const char *dir)
    {
        if (!main.get()) {
            return real.rmdir(dir);
        }
        std::string    realpath = path_master.Absolute(dir);
        FWL::Connector *cntr    = main->GetConnector(realpath);
        main->Logger().Inf("RmDir(%s) FWL::Connector(%p): ", dir, cntr);
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.rmdir(dir);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->RmDir(realpath);
    }

    int mkdir(const char *dir, mode_t mode)
    {
        if (!main.get()) {
            return real.mkdir(dir, mode);
        }
        std::string    realpath = path_master.Absolute(dir);
        FWL::Connector *cntr    = main->GetConnector(realpath);
        main->Logger().Inf("MkDir(%s) FWL::Connector(%p): ", dir, cntr);
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.mkdir(dir, mode);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->MkDir(realpath, mode);
        return 0;
    }

    DIR *opendir(const char *dir)
    {
        if (!main.get()) {
            return real.opendir(dir);
        }
        std::string    realpath = path_master.Absolute(dir);
        FWL::Connector *cntr    = main->GetConnector(realpath);
        main->Logger().Inf("OpenDir(%s) FWL::Connector(%p): ", dir, cntr);
        if (!cntr) {
            //! todo real write or something...think
            main->Logger().Inf("Call real function\n");
            return real.opendir(dir);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return (DIR *)cntr->OpenDir(realpath);
    }

    struct dirent *readdir(DIR *dir)
    {
        if (!main.get()) {
            return real.readdir(dir);
        }
        FWL::Connector *cntr = main->GetDirConnector(dir);
        main->Logger().Inf("ReadDir FWL::Connector(%p): ", cntr);
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.readdir(dir);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->ReadDir(dir);
    }

    int closedir(DIR *dir)
    {
        if (!main.get()) {
            return real.closedir(dir);
        }
        FWL::Connector *cntr = main->GetDirConnector(dir);
        main->Logger().Inf("CloseDir FWL::Connector(%p): ", cntr);
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.closedir(dir);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->CloseDir(dir);
    }

    int unlink(const char *path)
    {
        if (!main.get()) {
            return real.unlink(path);
        }
        std::string    realpath = path_master.Absolute(path);
        FWL::Connector *cntr    = main->GetConnector(realpath);
        main->Logger().Inf("Unlink FWL::Connector(%p): ", cntr);
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.unlink(path);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->Unlink(realpath);
    }

    int fcntl(int fd, int cmd, ...)
    {
        if (!main.get()) {
            VA_ARG(unsigned, p2, cmd);
            return (p2) ? real.fcntl(fd, cmd, p2) : real.fcntl(fd, cmd);
        }
        FWL::Connector *cntr = main->GetConnector(fd);
        main->Logger().Inf("Fcntl FWL::Connector(%p) %d - %s: ", cntr, fd, File(fd));
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            VA_ARG(unsigned, p2, cmd);
            return (p2) ? real.fcntl(fd, cmd, p2) : real.fcntl(fd, cmd);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return 0;
    }
}
