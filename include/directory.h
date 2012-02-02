#pragma once

extern "C" {
#include <dirent.h>
}

#include <vector>
#include <string>
#include "object.h"
namespace FWL {
    class Directory
    {
        private:
            int         fd_;
            size_t         index_;
            std::string name_;
            typedef std::vector<std::string>   FileList;
            FileList      files_;
            struct dirent dirent_;
        public:
            Directory(int fd, const std::string& name);
            const FileList& Files() const { return files_; }
            FileList& Files() { return files_; }
            void AddFile(const std::string& name);
            const std::string& Name() const { return name_; }
            int Fd() const { return fd_; }
            struct dirent *Read();
    };
}
