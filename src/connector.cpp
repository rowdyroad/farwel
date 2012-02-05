#include <boost/foreach.hpp>
#include "connector.h"
extern "C" {
#include <errno.h>
}
namespace FWL {
    Connector::Connector(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
        : name_(name)
        , fd_manager_(fd_manager)
        , config_(config)
        , log_(log)
    {
        JsonNodeConstOp log_node = config.get_child_optional("log");

        if (log_node) {
            std::auto_ptr<Log> nl(new Log(*log_));
            JsonNodeConstOp    level = log_node->get_child_optional("level");
            if (level) {
                nl->SetLogLevel(level->get_value<std::string>());
            }

            JsonNodeConstOp sinks = log_node->get_child_optional("sinks");
            if (sinks && !sinks->empty()) {
                nl->ClearSinks();
                BOOST_FOREACH(const JsonNode::value_type & it, *sinks)
                {
                    nl->UseSink(it.second.get_value<std::string>());
                }
            }
            log_.reset(nl.release());
        }
    }

    int Connector::openFile(FileIntr& file)
    {
        bool creat = (bool)(file->Flags() & O_CREAT);
        bool exist = Exists(file);         //! \todo: optimize it

        Logger().Dbg("%d %d - %d %d\n", file->Fd(), file->Flags(), creat, exist);
        if (!creat && !exist) {
            Logger().Dbg("Not for create and not exists");
            errno = ENOENT;
            return -1;
        }
        if (creat) {
            if (!exist) {
                Logger().Dbg("For create and not exists");
                if (!Create(file)) {
                    Logger().Dbg("Couldn't create");
                    errno = EACCES;
                    return -1;
                }
                return file->Fd();
            } else {
                Logger().Dbg("For create and exists");
                if (file->Flags() & O_EXCL) {
                    Logger().Dbg("Check for exists");
                    errno = EEXIST;
                    return -1;
                }
                if (file->Flags() & O_TRUNC) {
                    Logger().Dbg("Need to trunc");
                    if (!Truncate(file)) {
                        Logger().Dbg("Trunc error");
                        errno = EACCES;
                        return -1;
                    }
                    return file->Fd();
                }
            }
        }
        return file->Fd();
    }

    int Connector::Open(const std::string& path, int flags)
    {
        FileIntr file(new File(fd_manager_.Get(this), path, flags));
        int ret = openFile(file);
        if (ret < 0) {
            fd_manager_.Release(file->Fd(), this);
        }
        
        insert(file);
        return ret;
    }

    int Connector::Close(int fd)
    {
	Files::iterator it = files_.find(fd);
	if (it == files_.end()) {
	    return -1;
	}
	
        if (!Close(it->second)) {
    	    return -1;
        }
        fd_manager_.Release(fd, this);
        remove(it->second);
        return 0;
    }

    struct dirent *Connector::ReadDir(DIR *dd)
    {
        Directories::iterator it = dirs_.find(*(int *)dd);
        if (it == dirs_.end()) {
            return NULL;
        }
        return it->second->Read();
    }

    void *Connector::OpenDir(const std::string& name)
    {
        DirectoryIntr dir(new Directory(fd_manager_.Get(this), name));
        if (!Open(dir)) {
            fd_manager_.Release(dir->Fd(), this);
            return NULL;
        }
        insert(dir);
        return (void*)dir.get();
    }
    
    int Connector::Write(int fd, const void* data, size_t size)
    {
	Files::iterator it = files_.find(fd);
	if (it == files_.end()) {
	    return -1;
	}
	
	return Write(it->second, data, size);
    }
    
    int Connector::Read(int fd, void* data, size_t size)
    {
	Files::iterator it = files_.find(fd);
	if (it == files_.end()) {
	    return -1;
	}
	return Read(it->second, data, size);
    }

    int Connector::CloseDir(DIR *dd)
    {
        Directories::iterator it = dirs_.find(*(int *)dd);

        if (it == dirs_.end()) {
            return -1;
        }

        if (Close(it->second)) {
            fd_manager_.Release(it->first, this);
            remove(it->second);
            return 0;
        }
        return -1;
    }

    bool Connector::GetFileSize(int fd, size_t& size)
    {
        Files::iterator it = files_.find(fd);

        if (it == files_.end()) {
            return false;
        }
        return GetFileSize(it->second, size);
    }
}
