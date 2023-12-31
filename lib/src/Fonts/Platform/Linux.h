#ifndef __LINUXFONTS_H_
#define __LINUXFONTS_H_
#if __linux__
#include "Base.h"

namespace Fonts::Platform {
    namespace Linux {
        FontFileInfo FindFont(std::string file);
    }
} // namespace Fonts::Platform
#endif
#endif