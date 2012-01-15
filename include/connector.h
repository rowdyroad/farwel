#pragma once

extern "C" {
#include <dirent.h>
}

#include <vector>
#include <boost/optional.hpp>
#include "log.h"
#include "object.h"
#include "fdmanager.h"
#include "json.h"

class Connector
    : public Object
{
    protected:
        class DirFile
        {
            std::string name_;
            enum FileType {
                t_unknown,
                t_fifo,
                t_chr,
                t_dir,
                t_blk,
                t_reg,
                t_lnk,
                t_sock,
                t_wht
            } type_;

            public:
                DirFile(const std::string& name, FileType type = t_reg)
                    : name_(name)
                    , type_(type)
                {}

                const std::string& Name() const
                {
                    return(name_);
                }

                FileType Type() const
                {
                    return(type_);
                }
        };
        typedef std::vector<DirFile>   DirFiles;
    private:
        class DirFd
            : public Object
        {
            private:
                struct _dirdesc dir_;
                DirFiles        files_;
                int             index_;
                struct dirent   dirent_;
            public:

                DirFd(int fd)
                    : index_(-1)
                {
                    dir_.dd_fd = fd;
                }

                DirFiles& Files()
                {
                    return(files_);
                }

                struct dirent *Read()
                {
                    if (++index_ >= files_.size()) {
                        return(NULL);
                    }

                    DirFile& df = files_[index_];
                    dirent_.d_fileno = index_;
                    dirent_.d_type   = df.Type();
                    dirent_.d_namlen = df.Name().size();
                    ::memmove(&(dirent_.d_name), df.Name().data(), df.Name().size());
                    return(&dirent_);
                }

                struct _dirdesc *Dir()
                {
                    return(&dir_);
                }
        };

        std::string name_;
        FdManager&  fd_manager_;
        typedef boost::unordered_map<int, std::string>                    Keys;
        typedef boost::unordered_map<int, boost::intrusive_ptr<DirFd> >   Dirs;
        Keys            keys_;
        Dirs            dirs_;
        std::string     empty_key_;
        const JsonNode& config_;
        LogIntr         log_;
    protected:

        const std::string& GetKey(int fd) const
        {
            Keys::const_iterator it = keys_.find(fd);

            if (it == keys_.end()) {
                return(empty_key_);
            }
            return(it->second);
        }

        const JsonNode& Config() const
        {
            return(config_);
        }

        Log& Logger()
        {
            return(*log_);
        }

    public:
        Connector(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
            : name_(name)
            , fd_manager_(fd_manager)
            , config_(config)
            , log_(log)
        {
            JsonNodeConstOp node = config.get_child_optional("log");

            if (node) {
                std::auto_ptr<Log> nl(new Log(*log_));
                JsonNodeConstOp    level = config.get_child_optional("level");
                if (level) {
                    nl->SetLogLevel(level->get_value<std::string>());
                }

                JsonNodeConstOp sinks = config.get_child_optional("sinks");
                if (sinks || !sinks->empty()) {
                    nl->ClearSinks();
                    BOOST_FOREACH(const JsonNode::value_type & it, *sinks)
                    {
                        nl->UseSink(it.second.get_value<std::string>());
                    }
                }
            }
        }

        virtual ~Connector() {}

        const std::string& Name() const
        {
            return(name_);
        }

        int Open(const std::string& path, int flags)
        {
            int fd  = fd_manager_.Get(this);
            int ret = Open(fd, path, flags);

            if (ret < 0) {
                fd_manager_.Release(fd, this);
            }
            keys_.insert(std::make_pair(fd, path));
            return(ret);
        }

        int Close(int fd)
        {
            int r = CloseFd(fd);

            if (r < 0) {
                return(-1);
            }
            fd_manager_.Release(fd, this);
            keys_.erase(fd);
            return(r);
        }

        virtual int Write(int fd, const void *data, size_t size) = 0;
        virtual int Read(int fd, void *data, size_t size)        = 0;
        virtual int Unlink(const std::string& path) = 0;
        virtual int Rename(const std::string& name, const std::string& path)  { return(-1); }
        virtual int MkDir(const std::string& dir, int flags)  { return(-1); }
        virtual int RmDir(const std::string& str) { return(-1); }

        virtual struct dirent *ReadDir(DIR *dd)
        {
            Dirs::iterator it = dirs_.find(dd->dd_fd);

            if (it == dirs_.end()) {
                return(NULL);
            }
            return(it->second->Read());
        }

        DIR *OpenDir(const std::string& name)
        {
            std::auto_ptr<DirFd> fd(new DirFd(fd_manager_.Get(this)));

            if (!OpenDir(fd->Dir()->dd_fd, name, fd->Files())) {
                fd_manager_.Release(fd->Dir()->dd_fd, this);
                return(NULL);
            }
            return(dirs_.insert(std::make_pair(fd->Dir()->dd_fd, boost::intrusive_ptr<DirFd>(fd.release(), false))).first->second->Dir());
        }

        int CloseDir(DIR *fd)
        {
            Dirs::const_iterator it = dirs_.find(fd->dd_fd);

            if ((it == dirs_.end()) || (it->second->Dir() != fd)) {
                return(-1);
            }
            if (CloseDir(fd->dd_fd)) {
                fd_manager_.Release(fd->dd_fd, this);
                dirs_.erase(fd->dd_fd);
                return(0);
            }
            return(-1);
        }

    protected:
        virtual int Open(int fd, const std::string& path, int flags) = 0;
        virtual int CloseFd(int fd) = 0;

        virtual bool OpenDir(int dd, const std::string& path, DirFiles& files)  { return(false); }
        virtual bool CloseDir(int dd)  { return(-1); }
};

typedef boost::intrusive_ptr<Connector>   ConnectorIntr;

class ConnectorFactory
    : public Object
{
    public:
        virtual Connector *Create(const std::string& name, const JsonNode& node, FdManager& fd_manager, LogIntr log) = 0;
};

typedef boost::intrusive_ptr<ConnectorFactory>   ConnectorFactoryIntr;


#include "connectors/dummy.h"
#include "connectors/memory.h"
#include "connectors/db.h"
