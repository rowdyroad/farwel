#pragma once

extern "C" {
#include <dirent.h>
}

#include <vector>
#include "object.h"

        class Directory
        {
            private:
                int fd_;
                int index_;
                std::string name_;
                typedef std::vector<std::string> FileList;
                FileList files_;
                struct dirent dirent_;
            public:
                Directory(int fd, const std::string& name)
                    : fd_(fd)
                    , index_(-1)
                    , name_(name)
                {}

                const FileList& Files() const
                {
                    return files_;
                }

                void AddFile(const std::string& name)
                {
            	    files_.push_back(name);
                }

                const std::string& Name() const
                {
            	    return name_;
                }

                int Fd() const
                {
            	    return fd_;
                }

                struct dirent *Read()
                {
                    if (++index_ >= files_.size()) {
                        return NULL;
                    }

                    const std::string& name = files_[index_];
                    dirent_.d_fileno = index_;
                    dirent_.d_type   = df.Type();
                    dirent_.d_namlen = name.size();
                    ::memmove(&(dirent_.d_name), name.c_str(), name.size() + 1);
                    return &dirent_;
                }
        };