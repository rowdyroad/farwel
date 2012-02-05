#pragma once
#include <boost/intrusive_ptr.hpp>
extern "C" {
#include <dirent.h>
}

#include <vector>
#include <string>
#include "object.h"
namespace FWL {
    class Node
	: public Object
    {
	private:
	    int fd_;
	    std::string name_;
	public:
	    Node(int fd, const std::string& name)
		: fd_(fd)
		, name_(name)
	    {}
	    const std::string& Name() const { return name_; }
	    int Fd() const { return fd_; }
    };

    typedef boost::intrusive_ptr<Node> NodeIntr;

    class File
	: public Node
    {
        private:
            off_t      offset_;
            int flags_;
        public:
            File(int fd, const std::string& name, int flags);
            off_t Offset() const { return offset_; }
            int Flags() const { return flags_; }
    };

    typedef boost::intrusive_ptr<File> FileIntr;

    class Directory
	: public Node
    {
        private:
            size_t      index_;
            typedef std::vector<std::string>   FileList;
            FileList      files_;
            struct dirent dirent_;
        public:
            Directory(int fd, const std::string& name);
            const FileList& Files() const { return files_; }
            FileList& Files() { return files_; }
            void AddFile(const std::string& name);
            struct dirent *Read();
    };

    typedef boost::intrusive_ptr<Directory> DirectoryIntr;
}


