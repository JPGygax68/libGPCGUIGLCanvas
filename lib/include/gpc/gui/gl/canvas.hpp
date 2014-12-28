#pragma once

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <string>
#include <array>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/glew.h>

#include <gpc/gl/wrappers.hpp>
#include <gpc/gl/utils.hpp>
#include <gpc/fonts/RasterizedFont.hpp>

namespace gpc {

    namespace gui {

        // TODO: RGBFloat and RGBAFloat are implementation-independent and belong in
        // the module defining the Canvas concept

        struct RGBFloat {
            GLfloat r, g, b;
        };

        struct RGBAFloat {
            GLfloat r, g, b, a;
            RGBAFloat(): RGBAFloat { 0, 0, 0, 1 } {}
            RGBAFloat(GLfloat r_, GLfloat g_, GLfloat b_, GLfloat a_ = 1): r(r_), g(g_), b(b_), a(a_) {}
            auto operator / (float dsor) -> RGBAFloat & {
                r /= dsor, g /= dsor, b /= dsor, a /= dsor;
                return *this;
            }
        };

        inline auto 
        operator + (const RGBAFloat &color1, const RGBAFloat &color2) -> RGBAFloat 
        {
            return { color1.r + color2.r, color1.g + color2.g, color1.b + color2.b, color1.a + color2.a };
        }

        inline auto
        interpolate(const RGBAFloat &color1, const RGBAFloat &color2, float a) -> RGBAFloat 
        {
            RGBAFloat result;
            result.r = color1.r + a * (color2.r - color1.r);
            result.g = color1.g + a * (color2.g - color1.g);
            result.b = color1.b + a * (color2.b - color1.b);
            result.a = color1.a + a * (color2.a - color1.a);
            return result;
        }

        struct RGBA32 {
            uint8_t components[4];
        };

        inline auto 
        fromFloat(const RGBAFloat &from) -> RGBA32 
        {
            return { { uint8_t(from.r * 255), uint8_t(from.g * 255), uint8_t(from.b * 255), uint8_t(from.a * 255) } };
        }

        // TODO: use a struct from GPC Fonts ?
        struct GlyphHeight { 
            // TODO: use integer types with well-defined sizes
            int ascent, descent; 
        };

        namespace gl {

            class _CanvasBase {
            public:

                struct native_color_t { GLfloat components[4]; };

                typedef GLuint image_handle_t;

                typedef GLuint font_handle_t;

                auto rgb_to_native(const RGBFloat &color) -> native_color_t {
                    return native_color_t { { color.r, color.g, color.b, 1 } };
                }
                auto rgba_to_native(const RGBAFloat &color) -> native_color_t {
                    return native_color_t { { color.r, color.g, color.b, color.a } };
                }

                void prepare_context();

                void leave_context();

                void define_viewport(int x, int y, int width, int height);

                auto register_rgba_image(size_t width, size_t height, const RGBA32 *pixels) -> image_handle_t;

                void fill_rect(int x, int y, int w, int h, const native_color_t &color);

                void draw_image(int x, int y, int w, int h, image_handle_t image);

                void draw_image(int x, int y, int w, int h, image_handle_t image, int offset_x, int offset_y);

                void set_clipping_rect(int x, int y, int w, int h);

                void cancel_clipping();

                auto register_font(const gpc::fonts::RasterizedFont &rfont) -> font_handle_t;

                template <typename CharT>
                void draw_text(font_handle_t font, int x, int y, const CharT *text, size_t count);

            protected:

                _CanvasBase();

                void init(bool y_axis_downward);

                //void draw_rect(const GLint *v);

                void draw_rect(int x, int y, int width, int height);

            protected:

                struct ManagedFont: public gpc::fonts::RasterizedFont {
                    ManagedFont(const gpc::fonts::RasterizedFont &from): gpc::fonts::RasterizedFont(from) {}
                    std::vector<GLuint> buffer_textures;
                    std::vector<GLuint> textures; // one 1D texture per variant
                    GLuint vertex_buffer;
                    void storePixels();
                    template <bool YAxisDown> void createQuads();
                };

                static const std::string vertex_code, fragment_code;

                GLuint vertex_buffer, index_buffer;
                GLuint vertex_shader, fragment_shader;
                GLuint program;
                std::vector<GLuint> image_textures;
                std::vector<ManagedFont> managed_fonts;
                GLint vp_width, vp_height;
            };

            template <bool YAxisDown = false>
            class Canvas;
            
            template <>
            class Canvas<false>: public _CanvasBase {
            public:

                void init() { _CanvasBase::init(false); }

                void set_clipping_rect(int x, int y, int w, int h)
                {
                    EXEC_GL(glScissor, x, y, w, h);
                    EXEC_GL(glEnable, GL_SCISSOR_TEST);
                }
            };

            template <>
            class Canvas<true> : public _CanvasBase {
            public:

                void init() { _CanvasBase::init(true); }

                void set_clipping_rect(int x, int y, int w, int h)
                {
                    EXEC_GL(glScissor, x, vp_height - (y + h), w, h);
                    EXEC_GL(glEnable, GL_SCISSOR_TEST);
                }
            };

            // Method implementations -----------------------------------------

            template <typename CharT>
            void _CanvasBase::draw_text(font_handle_t handle, int x, int y, const CharT *text, size_t count)
            {
                using gpc::gl::setUniform;

                const auto &mfont = managed_fonts[handle];

                auto var_index = 0; // TODO: support multiple variants
                const auto &variant = mfont.variants[var_index]; 

                EXEC_GL(glEnableClientState, GL_VERTEX_ARRAY);
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, mfont.vertex_buffer);
                EXEC_GL(glVertexPointer, 2, GL_INT, 0, static_cast<GLvoid*>(0));

                EXEC_GL(glBindTexture, GL_TEXTURE_BUFFER, mfont.textures[var_index]); // font pixels

                {
                    auto glyph_index = mfont.findGlyph(*text);
                    const auto &glyph = variant.glyphs[glyph_index];
                    x -= glyph.cbox.x_min;
                }

                setUniform("render_mode", 5, 3);
                setUniform("font_pixels", 7, 0); // use texture unit 0 to access glyph pixels

                for (const auto *p = text; p < (text + count); p++) {

                    auto glyph_index = mfont.findGlyph(*p);
                    const auto &glyph = variant.glyphs[glyph_index];

                    setUniform("glyph_base", 8, glyph.pixel_base);
                    GLint cbox[4] = { glyph.cbox.x_min, glyph.cbox.x_max, glyph.cbox.y_min, glyph.cbox.y_max };
                    setUniform("glyph_cbox", 9, cbox);
                    GLint position[2] = { x, y };
                    setUniform("position", 10, position);

                    GLint base = 4 * glyph_index;
                    EXEC_GL(glDrawArrays, GL_QUADS, base, 4);

                    x += glyph.cbox.adv_x;
                }

                EXEC_GL(glBindTexture, GL_TEXTURE_BUFFER, 0);
                EXEC_GL(glDisableClientState, GL_VERTEX_ARRAY);
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, 0);
            }

            template <bool YAxisDown>
            inline void 
            _CanvasBase::ManagedFont::createQuads()
            {
                struct Vertex { GLint x, y; };

                EXEC_GL(glGenBuffers, 1, &vertex_buffer);

                // Compute total number of glyphs in font
                auto glyph_count = 0U;
                for (const auto &variant : variants) glyph_count += variant.glyphs.size();

                // Allocate a buffer big enough to hold each glyph as a quad or triangle strip
                std::vector<Vertex> vertices;
                vertices.reserve(4 * glyph_count);

                // Create a quad of 2D vertices (relative to insertion point) for each glyph
                for (const auto &variant : variants) {

                    // TODO: store and use the base index for each font variant

                    for (const auto &glyph : variant.glyphs) {
                        /* bottom left  */ vertices.emplace_back<Vertex>({ glyph.cbox.x_min, glyph.cbox.y_min });
                        /* bottom right */ vertices.emplace_back<Vertex>({ glyph.cbox.x_max, glyph.cbox.y_min });
                        /* top right    */ vertices.emplace_back<Vertex>({ glyph.cbox.x_max, glyph.cbox.y_max });
                        /* top left     */ vertices.emplace_back<Vertex>({ glyph.cbox.x_min, glyph.cbox.y_max });
                    }
                }

                // Upload the vertex array
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
                EXEC_GL(glBufferData, GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, 0); // just in case
            }

        } // ns gl
    } // ns gui
} // ns gpc