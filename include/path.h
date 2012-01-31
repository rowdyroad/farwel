#pragma once
extern "C" {
#include <unistd.h>
}
#include <string>
#include <vector>
#include <exception>

class Path
{
    private:
        std::string         current_path_;
        std::vector<int> slashes_;
    public:
        Path()
        {
            char buf[1024];

            if (!::getcwd(&buf[0], sizeof(buf))) {
                throw std::exception();
            }
            current_path_.assign(&buf[0]);
            for (size_t i = 0; i < current_path_.size(); ++i) {
                if (current_path_[i] == '/') {
                    slashes_.push_back(i);
                }
            }
        }

        const std::string& CurrentPath() const
        {
            return current_path_;
        }

        std::string Absolute(const std::string& file)
        {
            if (file[0] == '/') {
                return file;
            }
            std::string res;
            const char  *cc = file.c_str();
            if (file[0] == '.') {
                if (file[1] != '.') {
                    res.resize(current_path_.size() + file.size() - 1);
                    ::sprintf((char *)res.data(), "%s/%s", current_path_.c_str(), cc + 1);
                } else {
                    int    k   = slashes_.size();
                    size_t pos = 0;
                    do {
                        size_t p = file.find("../", pos);
                        if (p == std::string::npos) {
                            break;
                        }
                        pos = p + 3;
                        if (--k < 0) {
                            k = 0;
                        }
                    } while (1);
                    ::sprintf((char *)res.data(), "%.*s/%s", slashes_[k], current_path_.c_str(), cc + pos);
                }
            } else {
                res.resize(current_path_.size() + file.size() + 1);

                ::sprintf((char *)res.data(), "%s/%s", current_path_.c_str(), cc);
            }
            return res;
        }

        static std::string Directory(const std::string& file)
        {
            size_t pos = file.rfind("/");

            if (pos == std::string::npos) {
                return std::string();
            }
            return file.substr(0, pos);
        }

        static std::string File(const std::string& file)
        {
            size_t pos = file.rfind("/");

            if (pos == std::string::npos) {
                return std::string();
            }
            return std::string(file.begin() + pos + 1, file.end());
        }
};
