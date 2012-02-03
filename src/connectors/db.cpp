extern "C" {
#include <errno.h>
}
#include <boost/format.hpp>
#include "connectors/db.h"
namespace FWL {
    soci::session& Db::Session()
    {
        if (!session_.get()) {
            Logger().Dbg("ConnStr: %s\n", conn_str_.c_str());
            session_.reset(new soci::session(conn_str_));
        }
        return *session_;
    }

    bool Db::exists(const std::string& key)
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

    bool Db::create(const std::string& key)
    {
        static const std::string query = (boost::format("insert into `%1%` (`%2%`,`%3%`, `%4%`) value(:key, '', :parent)") % table_name_ % key_column_ % value_column_ % parent_column_).str();

        try {
            Logger().Dbg("Query:%s\n", query.c_str());
            soci::statement st((Session().prepare << query, soci::use(key, "key"), soci::use(Path::Directory(key), "parent")));
            st.execute();
            return st.get_affected_rows();
        } catch (const soci::soci_error& e) {
            Logger().Err("Create: %s", e.what());
            return false;
        }
    }

    bool Db::update(const std::string& key)
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

    bool Db::update(const std::string& key, const std::string& value)
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

    bool Db::append(const std::string& key, const std::string& value)
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

    bool Db::remove(const std::string& key)
    {
        static const std::string query = (boost::format("delete from `%1%` where `%2%` = :key ") % table_name_ % key_column_).str();
	static const std::string clear_query = (boost::format("delete from `%1` where `%2` = :key ") % table_name_ % parent_column_).str();

        try {
            Logger().Dbg("Query:%s key:%s", query.c_str(), key.c_str());
            soci::statement st((Session().prepare << query, soci::use(key)));
            st.execute();
            st.get_affected_rows();
            soci::statement clst((Session().prepare << clear_query, soci::use(key)));
            clst.execute();
            return true;
        } catch (const soci::soci_error& e) {
            Logger().Err("Remove: %s", e.what());
            return false;
        }
    }

    bool Db::read(const std::string& key, std::string& data)
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

    bool Db::readdir(const std::string& key, std::vector<std::string>& files)
    {
        static const std::string query = (boost::format("select `%2%` from `%1%` where `%3` = :parent ") % table_name_ % key_column_ % parent_column_).str();

        try {
            soci::rowset<std::string> rs = (Session().prepare << query, soci::use(key));

            files.clear();
            for (soci::rowset<std::string>::const_iterator it = rs.begin(); it != rs.end(); ++it) {
                files.push_back(Path::File(*it));
            }
            return true;
        } catch (const soci::soci_error& e) {
            Logger().Err("Readdir: %s", e.what());
            return false;
        }
    }

    bool Db::length(const std::string& key, size_t& size)
    {
        static const std::string query = (boost::format("select length(`%3%`) from `%2%` where `%2%` = :key") % table_name_ % key_column_ % value_column_).str();

        try {
            Session() << query, soci::use(key), soci::into(size);
            return true;
        } catch (const soci::soci_error& e) {
            Logger().Err("length: %s", e.what());
            return false;
        }
    }

    Db::Db(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
        : Connector(name, config, fd_manager, log)
        , conn_str_(config.get<std::string>("conn_str"))
        , table_name_(config.get<std::string>("table_name"))
        , key_column_(config.get<std::string>("key_column"))
        , value_column_(config.get<std::string>("value_column"))
        , parent_column_(config.get<std::string>("parent_column"))
    {}

    int Db::Open(int fd, const std::string& path, int flags)
    {
        errno = 0;
        bool creat = (bool)(flags & O_CREAT);
        bool exist = exists(path);         //! \todo: optimize it
        Logger().Dbg("%d %d - %d %d\n", fd, flags, creat, exist);
        if (!creat && !exist) {
            Logger().Dbg("Not for create and not exists");
            errno = ENOENT;
            return -1;
        }
        if (creat) {
            if (!exist) {
                Logger().Dbg("For create and not exists");
                if (!create(path)) {
                    Logger().Dbg("Couldn't create");
                    errno = EACCES;
                    return -1;
                }
                return fd;
            } else {
                Logger().Dbg("For create and exists");
                if (flags & O_EXCL) {
                    Logger().Dbg("Check for exists");
                    errno = EEXIST;
                    return -1;
                }
                if (flags & O_TRUNC) {
                    Logger().Dbg("Need to trunc");
                    if (!update(path)) {
                        Logger().Dbg("Trunc error");
                        errno = EACCES;
                        return -1;
                    }
                    return fd;
                }
            }
        }

        Logger().Dbg("Only for read");

        return fd;
    }

    int Db::MkDir(const std::string& path, mode_t mode)
    {
        return Open(0, path, O_CREAT);
    }

    int Db::Write(int fd, const void *data, size_t size)
    {
        Logger().Dbg("Write: %d\n", fd);
        const std::string& key = GetKey(fd);
        if (key.empty()) {
            return -1;
        }

        if (!append(key, std::string((const char *)data, size))) {
            return -1;
        }
        return size;
    }

    int Db::Read(int fd, void *data, size_t size)
    {
        const std::string& key = GetKey(fd);

        if (key.empty()) {
            return -1;
        }
        std::string str;
        if (!read(key, str)) {
            return -1;
        }
        size_t msize = std::min(size, str.size());
        ::memcpy(data, str.data(), msize);
        return msize;
    }

    bool Db::OpenDir(Directory& dir)
    {
        return readdir(dir.Name(), dir.Files());
    }

    int Db::CloseFd(int fd)
    {
        return 0;
    }

    bool Db::GetFileSize(const std::string& path, size_t& size)
    {
        return length(path, size);
    }

    int Db::Unlink(const std::string& path)
    {
        return remove(path);
    }

    int Db::RmDir(const std::string& path)
    {
        return Unlink(path);
    }

    Connector *DbFactory::Create(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
    {
        return new Db(name, config, fd_manager, log);
    }
}
