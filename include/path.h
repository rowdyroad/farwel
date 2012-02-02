#pragma once
#include <string>
#include <vector>
namespace FWL {
    class Path
    {
        private:
            std::string      current_path_;
            std::vector<int> slashes_;
        public:
            Path();

            const std::string& CurrentPath() const
            {
                return current_path_;
            }

            std::string Absolute(const std::string& file);
            static std::string Directory(const std::string& file);
            static std::string File(const std::string& file);
    };
}
