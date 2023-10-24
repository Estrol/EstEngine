#ifndef __FILESYSTEM_H_
#define __FILESYSTEM_H_

#ifdef WINAPI
#undef ReadFile
#endif

#include <filesystem>

namespace Misc {
    namespace Filesystem {
        std::vector<uint8_t> ReadFile(std::filesystem::path path);
        std::vector<uint16_t> ReadFile16(std::filesystem::path path);
    }
}

#endif