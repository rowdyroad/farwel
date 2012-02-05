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

    bool Db::Exists(FileIntr& file)
    {
        static const std::string query = (boost::format("select count(*) from `%1%` where `%2%` = :key") % table_name_ % key_column_).str();

        try {
            Logger().Dbg("Query: %s (key = %s)", query.c_str(), file->Name().c_str());
            int count = 0;
            Session() << query, soci::use(file->Name()), soci::into(count);
            return count > 0;
        } catch (const std::exception& e) {
            Logger().Err("Exists:%s", e.what());
            return false;
        }
    }

    bool Db::Create(FileIntr& file)
    {
        static const std::string query = (boost::format("insert into `%1%` (`%2%`,`%3%`,`%4%`) value(:key,'',:parent)") % table_name_ % key_column_ % value_column_ % parent_column_).str();

        try {
            Logger().Dbg("Query:%s - key:%s / parent: %s\n ", query.c_str(), file->Name().c_str(), Path::Directory(file->Name()).c_str());
            Session() << query, soci::use(file->Name()), soci::use(Path::Directory(file->Name()));
            return true;
        } catch (const soci::soci_error& e) {
            Logger().Err("Create: %s", e.what());
            return false;
        }
    }

    bool Db::Truncate(FileIntr& file)
    {
        static const std::string query = (boost::format("update `%1%` set `%3%` = '' where `%2%` = :key ") % table_name_ % key_column_ % value_column_).str();

        try {
            Logger().Dbg("Query:%s\n", query.c_str());
            soci::statement st((Session().prepare << query, soci::use(file->Name())));
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
        static const std::string query       = (boost::format("delete from `%1%` where `%2%` = :key ") % table_name_ % key_column_).str();
        static const std::string clear_query = (boost::format("delete from `%1%` where `%2%` = :key ") % table_name_ % parent_column_).str();

        try {
            Logger().Dbg("Query:%s key:%s", query.c_str(), key.c_str());
            soci::statement st((Session().prepare << query, soci::use(key)));
            st.execute();
            st.get_affected_rows();
            Logger().Dbg("Query:%s key:%s", clear_query.c_str(), key.c_str());
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
        static const std::string query = (boost::format("select `%2%` from `%1%` where `%3%` = :parent ") % table_name_ % key_column_ % parent_column_).str();

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

    int Db::Rename(const std::string& name, const std::string& newname)
    {
        static const std::string query     = (boost::format("update `%1%` set `%2%` = :newkey where `%2%` = :key") % table_name_ % key_column_).str();
        static const std::string dir_query = (boost::format("update `%1%` set `%2%` = :newkey where `%2%` = :key") % table_name_ % parent_column_).str();

        errno = 0;
        soci::statement st((Session().prepare << query, soci::use(name, ":key"), soci::use(newname, ":newkey")));
        st.execute();
        if (!st.get_affected_rows()) {
            errno = EEXIST;
            return -1;
        }
        soci::statement dirst((Session().prepare << dir_query, soci::use(name, ":key"), soci::use(newname, ":newkey")));
        dirst.execute();
        return 0;
    }

    int Db::MkDir(const std::string& path, mode_t mode)
    {
        return Connector::Open(path, O_CREAT);
    }

    bool Db::Close(DirectoryIntr& dir)
    {
        return true;
    }
    
    int Db::Write(FileIntr& file,const void *data, size_t size)
    {
        Logger().Dbg("Write: %d\n", file->Fd());
        if (!append(file->Name(), std::string((const char *)data, size))) {
            return -1;
        }
        return size;
    }

    int Db::Read(FileIntr& file,void *data, size_t size)
    {
	std::string str;
        if (!read(file->Name(), str)) {
            return -1;
        }
        size_t msize = std::min(size, str.size());
        ::memcpy(data, str.data(), msize);
        return msize;
    }

    bool Db::Open(DirectoryIntr& dir)
    {
        return readdir(dir->Name(), dir->Files());
    }

    bool Db::Close(FileIntr& file)
    {
        return 0;
    }

    bool Db::GetFileSize(FileIntr& file, size_t& size)
    {
        return length(file->Name(), size);
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
