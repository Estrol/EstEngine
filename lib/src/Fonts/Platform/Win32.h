#ifndef __WIN32FONTS_H_
#define __WIN32FONTS_H_
#if _WIN32
#include "Base.h"

namespace Fonts::Platform {
    namespace Win32 {
        FontFileInfo FindFont(std::string file);
    }
} // namespace Fonts::Platform

#endif
#endif