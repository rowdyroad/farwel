#pragma once

#include <vector>
#include <boost/format.hpp>
#include <soci/soci.h>
#include "../connector.h"

class Db
    : public Connector
{
    private:
	std::auto_ptr<soci::session> session_;
	typedef boost::unordered_map<int, std::string> Keys;
	Keys keys_;
	std::string conn_str_;
	std::string table_name_;
	std::string key_column_;
	std::string value_column_;
	soci::session& Session()
	{
	    if (!session_.get()) {
		Logger().Dbg("ConnStr: %s\n",conn_str_.c_str());
		session_.reset(new soci::session(conn_str_));
	    }
	    return *session_;
	}
	
	    bool exists(const std::string& key)
	    {
		static const std::string query = (boost::format("select count(*) from `%1%` where `%2%` = :key") %  table_name_ % key_column_).str();
		Logger().Dbg("Query:%s\n", query.c_str());
		soci::row row;
		Session() << query, soci::use(key), soci::into(row);
		return row.get<size_t>(0);
	    }
	    bool create(const std::string& key)
	    {
	    	static const std::string query = (boost::format("insert into `%1%` (`%2%`,`%3%`) value(:key, '')") % table_name_ % key_column_ % value_column_).str();
		Logger().Dbg("Query:%s\n", query.c_str());
		soci::statement st((Session().prepare << query, soci::use(key)));
		st.execute();
		return st.get_affected_rows();
	    }
	    bool update(const std::string& key)
	    {
		static const std::string query = (boost::format("update `%1%` set `%3%` = '' where `%2%` = :key ") % table_name_ % key_column_ % value_column_).str();
		Logger().Dbg("Query:%s\n", query.c_str());
		soci::statement st((Session().prepare << query, soci::use(key)));
		st.execute();
		return st.get_affected_rows();
	    }
	    
	    bool update(const std::string& key, const std::string& value)
	    {
		static const std::string query = (boost::format("update `%1%` set `%3%` = :value where `%2%` = :key ") % table_name_ % key_column_ % value_column_).str();
		Logger().Dbg("Query:%s\n", query.c_str());
		soci::statement st((Session().prepare << query, soci::use(key,"key"), soci::use(value,"value")));
		return st.get_affected_rows();
	    }
	    
	    bool append(const std::string& key, const std::string& value)
	    {
		static const std::string query = (boost::format("update `%1%` set `%3%` = CONCAT(`%3%`,:value) where `%2%` = :key ") % table_name_ % key_column_ % value_column_).str();
		Logger().Dbg("Query:%s\n", query.c_str());
		soci::statement st((Session().prepare << query, soci::use(key,"key"), soci::use(value,"value")));
		st.execute();
		return st.get_affected_rows();
	    }
	    
	    bool remove(const std::string& key)
	    {
		static const std::string query = (boost::format("delete from `%1%` where `%2%` = :key ") % table_name_ % key_column_).str();
		Logger().Dbg("Query:%s\n", query.c_str());
		soci::statement st((Session().prepare << query, soci::use(key)));
		st.execute();
		return st.get_affected_rows();
	    }
	    
	    bool read(const std::string& key, std::string& data)
	    {
	    	static const std::string query = (boost::format("select `%3%` from `%1%` where `%2%` = :key ") % table_name_ % key_column_ % value_column_).str();
		Logger().Dbg("Query:%s\n", query.c_str());
		Session() <<query, soci::into(data);
		return true;
	    }

    public:
	Db(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
	    : Connector(name, config, fd_manager, log)
	    , conn_str_(config.get<std::string>("conn_str"))
	    , table_name_(config.get<std::string>("table_name"))
	    , key_column_(config.get<std::string>("key_column"))
	    , value_column_(config.get<std::string>("value_column"))
	{
	    /*const JsonNode::value_type& log_node = config.get_child("log");
	    if (log_node) {
		log_.reset(new Log(log_node.et<Log::LogLevel>("level")));
		log_->AddSinks(log);
    		BOOST_FOREACH(const JsonNode::value_type &n, config.get_child("sinks")) {
		};
	    }*/
	}
	
	int Open(int fd, const std::string& path, int flags)
	{
	    Logger().Dbg("%d %d - %d\n", fd, (flags & O_CREAT), exists(path));
	    if ((bool)(flags & O_CREAT) == exists(path)) {
		return -1;
	    }
	    if (flags & O_CREAT) {
		Logger().Dbg("need to create\n");
		if (!create(path)) {
		    return -1;
		}
	    } else if (flags & O_TRUNC) {
	        Logger().Dbg("need trunc\n");
		if (!update(path)) {
	    	    return -1;
		}
	    }
	    keys_.insert(std::make_pair(fd, path));
	    return fd;
	}
	
	int Write(int fd, const void* data, size_t size)
	{
	    Logger().Dbg("Write: %d\n",fd);
	    Keys::const_iterator it = keys_.find(fd);
	    if (it == keys_.end()) {
		return -1;
	    }
	    if (!append(it->second, std::string((const char*)data,size))) {
		return -1;
	    } 
	    return size;
	}
	
	int Read(int fd, void* data, size_t size)
	{
	    Keys::const_iterator it = keys_.find(fd);
	    if (it == keys_.end()) {
		return -1;
	    }
	    
	    std::string str;
	    if (!read(it->second, str)) {
		return -1;
	    }
	    size_t msize = std::min(size, str.size());
	    ::memcpy(data, str.data(), msize);
	    return msize;
	}
	
	int CloseFd(int fd)
	{
	    keys_.erase(fd);
	    return 0;
	}
	
	int Unlink(const std::string& path)
	{
	    return remove(path);
	}
	
};


class DbFactory
    : public ConnectorFactory
{
    public:
	Connector* Create(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
	{
	    return new Db(name, config, fd_manager, log);
	}
};


