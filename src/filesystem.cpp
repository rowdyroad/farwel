#include <stdio.h>
#include <string.h>
#include "filesystem.h"

namespace FWL {
    Directory::Directory(int fd, const std::string& name)
        : fd_(fd)
        , index_(0)
        , name_(name)
    {}

    void Directory::AddFile(const std::string& name)
    {
        files_.push_back(name);
    }

    struct dirent *Directory::Read()
    {
        if (index_ >= files_.size()) {
            return NULL;
        }
        const std::string& name = files_[index_];
        dirent_.d_fileno = index_;
        dirent_.d_type   = DT_DIR | DT_REG;
#ifdef __linux__
        dirent_.d_reclen = name.size() + sizeof(dirent_) - 1;
#else
        dirent_.d_namlen = name.size();
#endif
        ::memmove(&(dirent_.d_name), name.c_str(), name.size() + 1);
        ++index_;
        return &dirent_;
    }
    
    File::File(int fd, const std::string& name, int flags)
	: Node(fd, name)
	, flags_(flags)
    {}
}


