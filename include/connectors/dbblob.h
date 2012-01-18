#pragma once

#include <vector>
#include <boost/format.hpp>
#include <soci/soci.h>
#include "../connector.h"
#include "../path.h"

class DbBlob
    : public Connector
{
    private:
        std::auto_ptr<soci::session> session_;
        typedef boost::shared_ptr<soci::blob> Blob;
        
        
        typedef boost::unordered_map<int,Blob> Blobs;
        Blobs blobs_;
        std::string conn_str_;
        std::string table_name_;
        std::string key_column_;
        std::string value_column_;

        soci::session& Session()
        {
            if (!session_.get()) {
                Logger().Dbg("ConnStr: %s\n", conn_str_.c_str());
                session_.reset(new soci::session(conn_str_));
            }
            return *session_;
        }
        
        Blob find(const std::string& key)
        {
	    static const std::string query = (boost::format("select `%3%` from `%1%` where `%2%` = :key") % table_name_ % key_column_ % value_column_).str();
    	    Blob b(new soci::blob(Session()));
    	    try 
    	    {
    		Session() << query, into(*b);
    	    } catch(...) {
    		b.reset();
    	    }
    	    return b;
        }

        bool exists(const std::string& key)
        {
            static const std::string query = (boost::format("select count(*) from `%1%` where `%2%` = :key") % table_name_ % key_column_).str();

            try {
                Logger().Dbg("Query:%s (key = %s)", query.c_str(), key.c_str());
                soci::row row;
                Session() << query, soci::use(key), soci::into(row);
                return row.get<size_t>(0);
            } catch (const std::exception& e) {
                Logger().Err("Exists:%s", e.what());
                return false;
            }
        }

        Blob create(const std::string& key)
        {
            static const std::string query = (boost::format("insert into `%1%` (`%2%`,`%3%`, `parent`) value(:key, '', :parent)") % table_name_ % key_column_ % value_column_).str();
            try {
                Logger().Dbg("Query:%s\n", query.c_str());
                soci::statement st((Session().prepare << query, soci::use(key, "key"), soci::use(Path::Directory(key), "parent")));
                st.execute();
                return find(key);
            } catch (const soci::soci_error& e) {
                Logger().Err("Create: %s", e.what());
                return Blob();
            }
        }

        bool update(const std::string& key)
        {
            static const std::string query = (boost::format("update `%1%` set `%3%` = '' where `%2%` = :key ") % table_name_ % key_column_ % value_column_).str();

            try {
                Logger().Dbg("Query:%s\n", query.c_str());
                soci::statement st((Session().prepare << query, soci::use(key)));
                st.execute();
                return st.get_affected_rows();
            } catch (const soci::soci_error& e) {
                Logger().Err("Update: %s", e.what());
                return false;
            }
        }

        bool update(const std::string& key, const std::string& value)
        {
            static const std::string query = (boost::format("update `%1%` set `%3%` = :value where `%2%` = :key ") % table_name_ % key_column_ % value_column_).str();

            try {
                Logger().Dbg("Query:%s\n", query.c_str());
                soci::statement st((Session().prepare << query, soci::use(key, "key"), soci::use(value, "value")));
                return st.get_affected_rows();
            } catch (const soci::soci_error& e) {
                Logger().Err("Update %s", e.what());
                return false;
            }
        }

        bool append(const std::string& key, const std::string& value)
        {
            static const std::string query = (boost::format("update `%1%` set `%3%` = CONCAT(`%3%`,:value) where `%2%` = :key ") % table_name_ % key_column_ % value_column_).str();

            try {
                Logger().Dbg("Query:%s\n", query.c_str());
                soci::statement st((Session().prepare << query, soci::use(key, "key"), soci::use(value, "value")));
                st.execute();
                return st.get_affected_rows();
            } catch (const soci::soci_error& e) {
                Logger().Err("Append: %s", e.what());
                return false;
            }
        }

        bool remove(const std::string& key)
        {
            static const std::string query = (boost::format("delete from `%1%` where `%2%` = :key ") % table_name_ % key_column_).str();

            try {
                Logger().Dbg("Query:%s key:%s", query.c_str(), key.c_str());
                soci::statement st((Session().prepare << query, soci::use(key)));
                st.execute();
                return st.get_affected_rows();
            } catch (const soci::soci_error& e) {
                Logger().Err("Remove: %s", e.what());
                return false;
            }
        }

        bool read(const std::string& key, std::string& data)
        {
            static const std::string query = (boost::format("select `%3%` from `%1%` where `%2%` = :key ") % table_name_ % key_column_ % value_column_).str();

            try {
                Logger().Dbg("Query:%s\n", query.c_str());
                Session() << query, soci::into(data);
                return true;
            } catch (const soci::soci_error& e) {
                Logger().Err("Read: %s", e.what());
                return false;
            }
        }

        bool readdir(const std::string& key, DirFiles& files)
        {
            static const std::string query = (boost::format("select `%2%` from `%1%` where `parent` = :parent ") % table_name_ % key_column_).str();

            try {
                soci::rowset<std::string> rs = (Session().prepare << query, soci::use(key));

                files.clear();
                for (soci::rowset<std::string>::const_iterator it = rs.begin(); it != rs.end(); ++it) {
                    files.push_back(DirFile(Path::File(*it)));
                }
                return true;
            } catch (const soci::soci_error& e) {
                Logger().Err("Readdir: %s", e.what());
                return false;
            }
        }

    public:
        DbBlob(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
            : Connector(name, config, fd_manager, log)
            , conn_str_(config.get<std::string>("conn_str"))
            , table_name_(config.get<std::string>("table_name"))
            , key_column_(config.get<std::string>("key_column"))
            , value_column_(config.get<std::string>("value_column"))
        {}

        int Open(int fd, const std::string& path, int flags)
        {
            errno = 0;
            bool creat = (bool)(flags & O_CREAT);
            Blob b = find(path); //! \todo: optimize it
            Logger().Dbg("%d %d - %d %p\n", fd, flags, creat, b.get());
            
            if (!creat && !b.get()) {
                Logger().Dbg("Not for create and not exists");
                errno = ENOENT;
                return -1;
            }
            if (creat) {
                if (!b.get()) {
                    Logger().Dbg("For create and not exists");
                    b = create(path);
                    if (!b.get()) {
                        Logger().Dbg("Couldn't create");
                        errno = EACCES;
                        return -1;
                    }
                } else {
                    Logger().Dbg("For create and exists");
                    if (flags & O_EXCL) {
                        Logger().Dbg("Check for exists");
                        errno = EEXIST;
                        return -1;
                    }
                    if (flags & O_TRUNC) {
                        Logger().Dbg("Need to trunc");
                        b->trim(0);
                    }
                }
            }
	    blobs_.insert(std::make_pair(fd, b));
            return fd;
        }

        int MkDir(const std::string& path, mode_t mode)
        {
            return Open(0, path, O_CREAT);
        }

        int Write(int fd, const void *data, size_t size)
        {
            Logger().Dbg("Write: %d\n", fd);
    	    Blobs::iterator it = blobs_.find(fd);
    	    if (it == blobs_.end()) {
    		return -1;
    	    }
    	    return it->second->append((const char*)data, size);
    	
        /*    const std::string& key = GetKey(fd);
            if (key.empty()) {
                return -1;
            }

            if (!append(key, std::string((const char *)data, size))) {
                return -1;
            }
            return size;*/
        }

        int Read(int fd, void *data, size_t size)
        {
    	    Blobs::iterator it = blobs_.find(fd);
    	    if (it == blobs_.end()) {
    		return -1;
    	    }
    	    return it->second->read(0, (char*)data, size);
            /*const std::string& key = GetKey(fd);

            if (key.empty()) {
                return -1;
            }
            std::string str;
            if (!read(key, str)) {
                return -1;
            }
            size_t msize = std::min(size, str.size());
            ::memcpy(data, str.data(), msize);
            return msize;*/
        }

        bool OpenDir(int fd, const std::string& path, DirFiles& files)
        {
            return readdir(path, files);
        }

        int CloseFd(int fd)
        {
    	    blobs_.erase(fd);
            return 0;
        }

        int Unlink(const std::string& path)
        {
            return remove(path);
        }

        int RmDir(const std::string& path)
        {
            return Unlink(path);
        }
};


class DbBlobFactory
    : public ConnectorFactory
{
    public:
        Connector *Create(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
        {
            return new DbBlob(name, config, fd_manager, log);
        }
};
