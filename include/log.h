#pragma once
#include <boost/intrusive_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <list>
#include <string>
extern "C" {
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
}
#include "object.h"

namespace FWL {
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
            const char *makeTime();
            std::string record(const std::string& sev, const std::string& format, va_list args);
            void print(const std::string& sev, const std::string& format, va_list args);
        public:
            Log(Level level);
            Log(const std::string& name);
            Level GetLevel() const;

            Log(const Log& log);
            void RegisterSink(const std::string& name, FILE *sink);
            void RegisterSink(const std::string& name, const std::string& filename);
            void SetLogLevel(Level level);
            void SetLogLevel(const std::string& name);
            void ClearSinks();
            bool UseSink(const std::string& name);
            ~Log();
            void Inf(const std::string& format, ...);
            void Dbg(const std::string& format, ...);
            void Wrn(const std::string& format, ...);
            void Err(const std::string& format, ...);
    };
}
