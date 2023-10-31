#ifndef __TEXT_H_
#define __TEXT_H_

#include "UIBase.h"
#include <Fonts/FontManager.h>

namespace UI {
    enum class Alignment {
        Left,
        Center,
        Right
    };

    class Text : public Base
    {
    public:
        Text();
        Text(std::string fontName, float fontSize = 16.0f);
        Text(Fonts::FontLoadFileInfo &info);
        Text(Fonts::FontLoadBufferInfo &info);

        void      DrawString(std::string text);
        void      DrawStringFormatted(std::string text, ...);
        glm::vec2 MeasureString(std::string text);

        float     Scale;
        Alignment Alignment;

    protected:
        void          OnDraw() override;
        Fonts::Glyph *FindGlyph(uint32_t c);

        std::string       m_TextToDraw;
        Fonts::FontAtlas *m_FontAtlas;
    };
} // namespace UI

#endif