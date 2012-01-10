#pragma once

#include <vector>
#include <libjson/libjson.h>
#include "../connector.h"

extern "C" {
#include <sys/types.h>
}

class Memory
    : public Connector
{
    private:
	class File
	{
	    private:
		std::string filename_;
		size_t fd_;
		std::vector<uint8_t> data_;
	    public:
		const std::string& Filename() const
		{
		    return filename_;
		}
		
		const size_t Fd() const
		{
		    return fd_;
		}
		
		const std::vector<uint8_t>& Data() const
		{
		    return data_;
		}

		std::vector<uint8_t>& Data()
		{
		    return data_;
		}
	};
	typedef boost::unordered_map<std::string, boost::intrusive_ptr<File> > Files;
	Files files_;
	typedef boost::unoredred_map<size_t, boost::intrusive_ptr<File> > FilesD;
	FilesD files_d_;
	
    public:
	int Open(const std::string& path, int flags)
	{
	    if (flag & O_CREAT) {
		files_[path] = boost::intrusive_ptr<File>(new File(
	    }
	    Files::const_iterator it = pat
	}
	
	size_t Write(int fd, const void* data, size_t size)
	{
	}
	
	size_t Read(int fd, void* data, size_t size)
	{ }
	
	size_t Close(int fd)
	{ }
	
	size_t Unlink(const std::string& path)
	{ }
};
