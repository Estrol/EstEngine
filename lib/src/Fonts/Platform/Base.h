#ifndef __PLATFORMBASE_H_
#define __PLATFORMBASE_H_
#include <filesystem>
#include <string>

namespace Fonts::Platform {
    struct FontFileInfo
    {
        std::filesystem::path path;
        bool                  error;
    };
} // namespace Fonts::Platform

#endif