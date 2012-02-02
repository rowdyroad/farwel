#include "log.h"

#define PRINT(lev, lev_txt)       \
    if (level_ < lev) { return; } \
    va_list args;                 \
    va_start(args, format);       \
    print(lev_txt, format, args);

namespace FWL {
    const char *Log::makeTime()
    {
        time_t t = ::time(NULL);

        if (last_time_ == t) {
            return &time_[0];
        }
        struct tm *lt = ::localtime(&t);
        ::strftime(&time_[0], sizeof(time_), "%Y-%m-%d %H:%M:%S", lt);
        return &time_[0];
    }

    std::string Log::record(const std::string& sev, const std::string& format, va_list args)
    {
        char        buf[1024];
        size_t      size = ::vsprintf(&buf[0], format.c_str(), args);
        std::string res(size + sizeof(time_) + sev.size() + 5, 0);

        ::sprintf((char *)res.data(), "[%s] %s: %s\n", makeTime(), sev.c_str(), &buf[0]);
        return res;
    }

    void Log::print(const std::string& sev, const std::string& format, va_list args)
    {
        std::string rec(record(sev, format, args));

        for (Sinks::iterator it = sinks_.begin(); it != sinks_.end(); ++it) {
            ::fprintf(*it, "%s", rec.c_str());
            ::fflush(*it);
        }
    }
	Log::Log(Level level)
            : level_(level)
        {}

        Log::Log(const std::string& name)
        {
            SetLogLevel(name);
        }

        Log::Level Log::GetLevel() const
        {
            return level_;
        }

        Log::Log(const Log& log)
            : registered_sinks_(log.registered_sinks_)
            , sinks_(log.sinks_)
            , level_(log.level_)
        {}

        void Log::RegisterSink(const std::string& name, FILE *sink)
        {
            registered_sinks_.insert(std::make_pair(name, sink));
        }

        void Log::RegisterSink(const std::string& name, const std::string& filename)
        {
            FILE *f = ::fopen(filename.c_str(), "w+");

            if (f) {
                RegisterSink(name, f);
                sink_files_.push_back(f);
            }
        }

        void Log::SetLogLevel(Log::Level level)
        {
            level_ = level;
        }

        void Log::SetLogLevel(const std::string& name)
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

        void Log::ClearSinks()
        {
            sinks_.clear();
        }

        bool Log::UseSink(const std::string& name)
        {
            SinkList::const_iterator it = registered_sinks_.find(name);

            if (it == registered_sinks_.end()) {
                return false;
            }
            sinks_.push_back(it->second);
            return true;
        }

        Log::~Log()
        {
            for (Sinks::iterator it = sink_files_.begin(); it != sink_files_.end(); ++it) {
                ::fclose(*it);
            }
        }

        void Log::Inf(const std::string& format, ...)
        {
            PRINT(Info, "INF");
        }

        void Log::Dbg(const std::string& format, ...)
        {
            PRINT(Debug, "DBG");
        }

        void Log::Wrn(const std::string& format, ...)
        {
            PRINT(Warn, "WRN");
        }

        void Log::Err(const std::string& format, ...)
        {
            PRINT(Error, "ERR");
        }
}
