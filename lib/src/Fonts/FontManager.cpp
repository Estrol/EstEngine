#include "FontManager.h"
#include <SDL2/SDL.h>

#include <Exceptions/EstException.h>
#include <Graphics/NativeWindow.h>
#include <Graphics/Renderer.h>
#include <Misc/Filesystem.h>
#include <freetype/config/ftheader.h>
#include FT_FREETYPE_H

#if _WIN32
#include "./Platform/Win32.h"
#endif

#if __linux__
#include "./Platform/Linux.h"
#endif

using namespace Fonts;

FontManager *FontManager::m_instance = nullptr;

FontManager *FontManager::Get()
{
    if (m_instance == nullptr) {
        m_instance = new FontManager();
    }
    return m_instance;
}

void FontManager::Destroy()
{
    if (m_instance != nullptr) {
        delete m_instance;
        m_instance = nullptr;
    }
}

FontManager::FontManager()
{
}

FontManager::~FontManager()
{
    auto renderer = Graphics::Renderer::Get();

    for (auto &font : m_fonts) {
        font->Texture.reset();
    }

    m_fonts.clear();
}

FontAtlas *FontManager::LoadFont(FontLoadFileInfo &info)
{
    auto path = info.Path;
    if (!std::filesystem::exists(path)) {
#if _WIN32
        auto fontInfo = Platform::Win32::FindFont(path.string());

#elif __linux__
        auto fontInfo = Platform::Linux::FindFont(path.string());
#else
        auto fontInfo = Fonts::Platform::FontFileInfo();
        fontInfo.error = true;
#endif

        if (fontInfo.error) {
            throw Exceptions::EstException("Could not find font");
        } else {
            path = fontInfo.path;
        }
    }

    FontLoadBufferInfo _info = {};
    _info.Buffer = Misc::Filesystem::ReadFile(path);
    _info.FontSize = info.FontSize;
    _info.Ranges = info.Ranges;

    return LoadFont(_info);
}

FontAtlas *FontManager::LoadFont(FontLoadBufferInfo &info)
{
    // TODO: add check if already in cache

    FT_Library ft;
    if (FT_Init_FreeType(&ft) != FT_Err_Ok) {
        throw Exceptions::EstException("Could not init FreeType library");
    }

    std::vector<Glyph> glyphs;

    FT_Face face;
    if (FT_New_Memory_Face(ft, (const FT_Byte *)info.Buffer.data(), (FT_ULong)info.Buffer.size(), 0, &face) != FT_Err_Ok) {
        throw Exceptions::EstException("Failed to load font");
    }

    SDL_Window *window = Graphics::NativeWindow::Get()->GetWindow();
    float       dpi = 0;
    SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window), nullptr, &dpi, nullptr);

    FT_UInt fontPixel = static_cast<FT_UInt>(::floor(info.FontSize * dpi / 72.0f));

    FT_Set_Pixel_Sizes(face, fontPixel, fontPixel);

    // calculate texture size
    float num_glyphs = 0;
    for (auto &range : info.Ranges) {
        uint32_t cmin = range.first;
        uint32_t cmax = range.second;

        num_glyphs += cmax - cmin;
    }

    uint32_t max_dim = static_cast<uint32_t>((1 + (face->size->metrics.height >> 6)) * ceilf(sqrtf((float)num_glyphs)));
    uint32_t tex_width = 1;
    while (tex_width < max_dim) {
        tex_width <<= 1;
    }

    uint32_t tex_height = tex_width;
    Glyph    Invalid;

    std::vector<unsigned char> tex_data(tex_width * tex_height, 0);
    uint32_t                   pen_x = 0, pen_y = 0;
    float                      faceHeight = info.FontSize * face->height / face->units_per_EM;

    for (auto &range : info.Ranges) {
        uint32_t cmin = range.first;
        uint32_t cmax = range.second;

        for (uint32_t c = cmin; c < cmax; c++) {
            FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL);
            FT_Bitmap *bmp = &face->glyph->bitmap;

            if (pen_x + bmp->width >= tex_width) {
                pen_x = 0;
                pen_y += 1 + (face->size->metrics.height >> 6);
            }

            if (pen_y + bmp->rows >= tex_height) {
                throw Exceptions::EstException("Font atlas is too small");
            }

            for (uint32_t row = 0; row < bmp->rows; ++row) {
                for (uint32_t col = 0; col < bmp->width; ++col) {
                    uint32_t x = pen_x + col;
                    uint32_t y = pen_y + row;
                    tex_data[y * tex_width + x] = bmp->buffer[row * bmp->pitch + col];
                }
            }

            Glyph glyph;
            glyph.Character = c;
            glyph.Size = glm::vec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
            glyph.Bearing = glm::vec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
            glyph.Advance = face->glyph->advance.x >> 6;

            glyph.Top = glm::vec2(pen_x, pen_y);
            glyph.Bottom = glm::vec2(pen_x + bmp->width, pen_y + bmp->rows);
            glyph.FaceHeight = static_cast<float>(face->height / 64);

            int ascender = face->size->metrics.ascender >> 6;
            int descender = face->size->metrics.descender >> 6;

            glyph.Ascender = static_cast<float>(ascender);
            glyph.Descender = static_cast<float>(descender);

            float x0 = static_cast<float>(face->glyph->bitmap_left);
            float y0 = static_cast<float>(-face->glyph->bitmap_top);
            float x1 = x0 + face->glyph->bitmap.width;
            float y1 = y0 + face->glyph->bitmap.rows;

            glyph.Rect = glm::vec4(x0, y0, x1, y1);

            glyph.UV[0] = glm::vec2(glyph.Top.x / tex_width, glyph.Top.y / tex_height);
            glyph.UV[1] = glm::vec2(glyph.Bottom.x / tex_width, glyph.Top.y / tex_height);
            glyph.UV[2] = glm::vec2(glyph.Bottom.x / tex_width, glyph.Bottom.y / tex_height);
            glyph.UV[3] = glm::vec2(glyph.Top.x / tex_width, glyph.Bottom.y / tex_height);

            if (c == '?') {
                Invalid = glyph;
            }

            glyphs.push_back(glyph);
            pen_x += bmp->width + 1;
        }
    }

    FT_Done_Face(face);

    std::vector<unsigned char> rgba_data(tex_width * tex_height * 4, 0);
    for (int i = 0; i < tex_data.size(); i++) {
        rgba_data[i * 4 + 0] |= tex_data[i];
        rgba_data[i * 4 + 1] |= tex_data[i];
        rgba_data[i * 4 + 2] |= tex_data[i];
        rgba_data[i * 4 + 3] |= 0xFF;
    }

    auto tex = Graphics::Renderer::Get()->LoadTexture(
        (const char *)rgba_data.data(),
        tex_width,
        tex_height);

    auto atlas = std::make_unique<FontAtlas>();
    atlas->Texture = std::unique_ptr<Graphics::Texture2D>(tex);
    atlas->Glyphs = std::move(glyphs);
    atlas->NewlineHeight = faceHeight;
    atlas->TexSize = glm::vec2(tex_width, tex_height);
    atlas->Invalid = Invalid;
    atlas->FontSize = info.FontSize;

    m_fonts.push_back(std::move(atlas));

    return m_fonts.back().get();
}