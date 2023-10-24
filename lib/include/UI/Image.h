#ifndef __IMAGE_H_
#define __IMAGE_H_

#include "UIBase.h"
#include <filesystem>

namespace UI {
    class Image : public Base
    {
    public:
        Image();
        Image(std::filesystem::path path);
        Image(const char* buf, size_t size);
        Image(const char* pixbuf, uint32_t width, uint32_t height);

    protected:
        void OnDraw() override;
    };
}

#endif