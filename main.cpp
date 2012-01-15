#include "include/real.h"
#include "include/main.h"
#include "include/path.h"

#include <assert.h>
#include "include/debug.h"
static Real                real;
static std::auto_ptr<Main> main;
static Path                path_master;

extern "C" {
    void __attribute__((constructor)) init(void)
    {
        const char *config_file = getenv("FRWL_CONFIG_FILE");

        if (!config_file) {
            fprintf(stderr, "Config file is not set. Set env param FRWL_CONFIG_FILE\n");
            exit(-1);
        }
        main.reset(new Main(config_file));
    }

    int open(const char *path, int flags, ...)
    {
        if (!main.get()) {
            return real.open(path, flags);
        }
        std::string realpath = path_master.Absolute(path);
        Connector   *cntr    = main->GetPathConnector(realpath);
        main->Logger().Inf("Open Connector(%p): %s", cntr, realpath.c_str());
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.open(path, flags);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
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
        Connector *cntr = main->GetConnector(fd);
        main->Logger().Inf("Write Connector(%p): ", cntr);
        if (!cntr) {
            //! todo real write or something...think
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
        Connector *cntr = main->GetConnector(fd);
        main->Logger().Inf("Write Connector(%p) %d: ", cntr, fd);
        if (!cntr) {
            //! todo real write or something...think
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
        Connector *cntr = main->GetConnector(fd);
        main->Logger().Inf("Read Connector(%p) %d: ", cntr, fd);
        if (!cntr) {
            //! todo real write or something...think
            main->Logger().Inf("Call real function\n");
            return real.read(fd, data, size);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->Read(fd, data, size);
    }

/*
 * int stat(const char* file, struct stat* buf)
 * {
 *  main->Logger().Inf("Stat %s\n", file);
 *  return -1;
 * }
 *
 * int fstat(int fd, struct stat *buf)
 * {
 *  main->Logger().Inf("Stat %d\n", fd);
 *  return -1;
 * }
 *
 * int lstat(const char *file, struct stat *buf)
 * {
 *  main->Logger().Inf("Lstat %s\n", file);
 *  return -1;
 * }
 *
 * int link(const char* file, const char* link)
 * {
 *  main->Logger().Inf("link %s -> %s\n", file, link);
 *  return -1;
 * }
 *
 * int symlink(const char* file, const char* link)
 * {
 *  main->Logger().Inf("symlink %s -> %s\n", file, link);
 *  return -1;
 * }
 *
 * int rename(const char* name, const char* newname)
 * {
 *  main->Logger().Inf("rename %s -> %s\n", name, newname);
 *  return -1;
 * }*/
    int rmdir(const char *dir)
    {
        if (!main.get()) {
            return real.rmdir(dir);
        }
        std::string realpath = path_master.Absolute(dir);
        Connector   *cntr    = main->GetPathConnector(realpath);
        main->Logger().Inf("RmDir(%s) Connector(%p): ", dir, cntr);
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
        std::string realpath = path_master.Absolute(dir);
        Connector   *cntr    = main->GetPathConnector(realpath);
        main->Logger().Inf("MkDir(%s) Connector(%p): ", dir, cntr);
        if (!cntr) {
            //! todo real write or something...think
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
        std::string realpath = path_master.Absolute(dir);
        Connector   *cntr    = main->GetPathConnector(realpath);
        main->Logger().Inf("OpenDir(%s) Connector(%p): ", dir, cntr);
        if (!cntr) {
            //! todo real write or something...think
            main->Logger().Inf("Call real function\n");
            return real.opendir(dir);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->OpenDir(realpath);
    }

    struct dirent *readdir(DIR *dir)
    {
        if (!main.get()) {
            return real.readdir(dir);
        }
        Connector *cntr = main->GetConnector(dir);
        main->Logger().Inf("ReadDir Connector(%p): ", cntr);
        if (!cntr) {
            //! todo real write or something...think
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
        Connector *cntr = main->GetConnector(dir);
        main->Logger().Inf("CloseDir Connector(%p): ", cntr);
        if (!cntr) {
            //! todo real write or something...think
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
        std::string realpath = path_master.Absolute(path);
        Connector   *cntr    = main->GetPathConnector(realpath);
        main->Logger().Inf("Unlink Connector(%p): ", cntr);
        if (!cntr) {
            main->Logger().Inf("Call real function\n");
            return real.unlink(path);
        }
        main->Logger().Inf("%s\n", cntr->Name().c_str());
        return cntr->Unlink(realpath);
    }
}
