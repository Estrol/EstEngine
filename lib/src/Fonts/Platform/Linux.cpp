#if __linux__
#include "Linux.h"
using namespace Fonts::Platform;

FontFileInfo Linux::FindFont(std::string file)
{
#ifndef __linux__
    return { "", true };
#else
    std::filesystem::path usr_share_fonts = "/usr/share/fonts/";
    std::filesystem::path usr_local_share_fonts = "/usr/local/share/fonts/";
    std::filesystem::path home_fonts = std::filesystem::path(getenv("HOME")) / ".fonts/";

    for (const auto &baseDir : { usr_share_fonts, usr_local_share_fonts, home_fonts }) {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(baseDir)) {
            if (entry.is_regular_file() && entry.path().filename() == fontName) {
                return { entry.path(), false };
            }
        }
    }

    return { "", true };
#endif
}
#endif