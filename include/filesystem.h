#pragma once
#include "object.h"

class Connector;

class Entity
    : public Object
{
    private:
	int fd_;
	std::string name_;
    public:
	Entity(const std::string& name, int fd)
	    : name_(name)
	    , fd_(fd)
	{};
	
	Entity(const std::string& name)
	    : name_(name)
	    , fd_(-1)
	{}
	
	virtual ~Entity() {};
	
	int Fd() const
	{
	    return fd_;
	}
	
	const std::string& Name() const 
	{
	    return name_;
	}
	
	virtual int Unlink() = 0;
	virtual int Rename(const std::string& name) = 0;
	virtual int CloseFd() = 0;
	
	virtual int Stat() = 0;
	
	int Close()
	{
	    int ret = CloseFd();
	    if (!ret) {
		connector_->Manager().Release(fd_);
	    }
	    return ret;
	}
};

typedef boost::intrusive_ptr<Entity> EntityIntr;

class File
    : public Enity
{
    public:
	File(const std::string& name, int fd)
	    : Entity(name, fd)
	{}
	File(const std::string& name)
	    : Entity(name)
	{}
	
	virtual size_t Offset() const = 0;
	virtual size_t Size() const = 0;
	virtual int Write(const void* data, size_t size) = 0;
	virtual int Read(void* data, size_it size) = 0;
	virtual int Truncate(size_t length) = 0;
};

typedef boost::inrusive_ptr<File> FileIntr;

class Directory
    : public Entity
{
    public:
	struct Description {
	    int fd;
	    Directory* ptr;
	    Description(int fd, Directory* ptr)
		: fd(fd)
		, ptr(ptr)
	    {}
	};
	
    protected:
	std::vector<std::string> files_;
	DirDesc desc_;
	off_t index_;
	struct dirent dirent_;
    public:
	Directory(const std::string& name, int fd)
	    : Entity(name, fd)
	    , desc_(fd, this)
	    , index_(0)
	{}
	
	struct dirent* ReadDir()
	{
	    if (++index >= files_.end()) {
		return NULL;
	    }
	}
	
	void Rewind() {
	    LoadFiles(files_);
	    index_ = 0;
	}
	void Seek(off_t offset) 
	{
	    LoadFiles(files_);
	    index_ = offset;
	}
	
	off_t Tell() {
	    return index_;
	}
	
	DIR* Dir() const
	{
	    return (DIR*)&desc_;
	}
	
	virtual int LoadFiles(std::vector<std::string>& files) {
	    return 0;
	};
};


typedef boost::intrusive_ptr<Directory> DirectoryIntr;
