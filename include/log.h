#pragma once
extern "C" {
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
}
#include <string>

#define PRINT(lev, lev_txt)       \
    if (level_ < lev) { return; } \
    va_list args;                 \
    va_start(args, format);       \
    print(lev_txt, format, args);

class Log;
typedef boost::intrusive_ptr<Log>   LogIntr;

class Log
    : public Object
{
    public:
        enum Level {
            None,
            Error,
            Warn,
            Debug,
            Info
        };

    private:
        typedef boost::unordered_map<std::string, FILE *>   SinkList;
        SinkList registered_sinks_;
        typedef std::list<FILE *>                           Sinks;
        Sinks  sink_files_;
        Sinks  sinks_;
        Level  level_;
        char   time_[19];
        time_t last_time_;
        const char *makeTime()
        {
            time_t t = ::time(NULL);

            if (last_time_ == t) {
                return &time_[0];
            }
            struct tm *lt = ::localtime(&t);
            ::strftime(&time_[0], sizeof(time_), "%Y-%m-%d %H:%M:%S", lt);
            return &time_[0];
        }

        std::string record(const std::string& sev, const std::string& format, va_list args)
        {
            char        buf[1024];
            size_t      size = ::vsprintf(&buf[0], format.c_str(), args);
            std::string res(size + sizeof(time_) + sev.size() + 5, 0);

            ::sprintf((char *)res.data(), "[%s] %s: %s\n", makeTime(), sev.c_str(), &buf[0]);
            return res;
        }

        void print(const std::string& sev, const std::string& format, va_list args)
        {
            std::string rec(record(sev, format, args));

            for (Sinks::iterator it = sinks_.begin(); it != sinks_.end(); ++it) {
                ::fprintf(*it, rec.c_str());
                ::fflush(*it);
            }
        }

    public:
        Log(Level level)
            : level_(level)
        {}

        Log(const std::string& name)
        {
            SetLogLevel(name);
        }

        Level GetLevel() const
        {
            return level_;
        }

        Log(const Log& log)
            : registered_sinks_(log.registered_sinks_)
            , sinks_(log.sinks_)
            , level_(log.level_)
        {}

        void RegisterSink(const std::string& name, FILE *sink)
        {
            registered_sinks_.insert(std::make_pair(name, sink));
        }

        void RegisterSink(const std::string& name, const std::string& filename)
        {
            FILE *f = ::fopen(filename.c_str(), "w+");

            if (f) {
                RegisterSink(name, f);
                sink_files_.push_back(f);
            }
        }

        void SetLogLevel(Level level)
        {
            level_ = level;
        }

        void SetLogLevel(const std::string& name)
        {
            if (name == "none") {
                level_ = None;
            } else if (name == "error") {
                level_ = Error;
            } else if (name == "warn") {
                level_ = Warn;
            } else if (name == "debug") {
                level_ = Debug;
            } else if (name == "info") {
                level_ = Info;
            }
        }

        void ClearSinks()
        {
            sinks_.clear();
        }

        bool UseSink(const std::string& name)
        {
            SinkList::const_iterator it = registered_sinks_.find(name);

            if (it == registered_sinks_.end()) {
                return false;
            }
            sinks_.push_back(it->second);
            return true;
        }

        ~Log()
        {
            for (Sinks::iterator it = sink_files_.begin(); it != sink_files_.end(); ++it) {
                ::fclose(*it);
            }
        }

        void Inf(const std::string& format, ...)
        {
            PRINT(Info, "INF");
        }

        void Dbg(const std::string& format, ...)
        {
            PRINT(Debug, "DBG");
        }

        void Wrn(const std::string& format, ...)
        {
            PRINT(Warn, "WRN");
        }

        void Err(const std::string& format, ...)
        {
            PRINT(Error, "ERR");
        }
};
