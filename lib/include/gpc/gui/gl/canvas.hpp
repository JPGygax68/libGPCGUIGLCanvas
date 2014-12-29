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

            template <
                bool YAxisDown
            >
            class Canvas {
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

                Canvas();

                void init();

                void draw_rect(int x, int y, int width, int height);

            private:

                // TODO: move this back into non-template base class

                static auto vertex_code() -> std::string {

                    return std::string {
                        #include "vertex.glsl.h"
                    };
                }

                static auto fragment_code() -> std::string {

                    return std::string {
                        #include "fragment.glsl.h"
                    };
                }

                struct ManagedFont: public gpc::fonts::RasterizedFont {
                    ManagedFont(const gpc::fonts::RasterizedFont &from): gpc::fonts::RasterizedFont(from) {}
                    std::vector<GLuint> buffer_textures;
                    std::vector<GLuint> textures; // one 1D texture per variant
                    GLuint vertex_buffer;
                    void storePixels();
                    void createQuads();
                };

                //static const std::string vertex_code, fragment_code;

                GLuint vertex_buffer, index_buffer;
                GLuint vertex_shader, fragment_shader;
                GLuint program;
                std::vector<GLuint> image_textures;
                std::vector<ManagedFont> managed_fonts;
                GLint vp_width, vp_height;

            };

            // Method implementations -----------------------------------------

            template <bool YAxisDown>
            Canvas<YAxisDown>::Canvas() :
                vertex_buffer(0), index_buffer(0),
                vertex_shader(0), fragment_shader(0), program(0)
            {
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::init()
            {
                static std::once_flag flag;
                std::call_once(flag, []() { glewInit(); });

                // Upload and compile our shader program
                {
                    assert(vertex_shader == 0);
                    vertex_shader = CALL_GL(glCreateShader, GL_VERTEX_SHADER);
                    // TODO: dispense with the error checking and logging in release builds
                    auto log = gpc::gl::compileShader(vertex_shader, vertex_code(), YAxisDown ? "#define Y_AXIS_DOWN" : "");
                    if (!log.empty()) std::cerr << "Vertex shader compilation log:" << std::endl << log << std::endl;
                }
                {
                    assert(fragment_shader == 0);
                    fragment_shader = CALL_GL(glCreateShader, GL_FRAGMENT_SHADER);
                    // TODO: dispense with the error checking and logging in release builds
                    auto log = gpc::gl::compileShader(fragment_shader, fragment_code(), YAxisDown ? "#define Y_AXIS_DOWN" : "");
                    if (!log.empty()) std::cerr << "Fragment shader compilation log:" << std::endl << log << std::endl;
                }
                assert(program == 0);
                program = CALL_GL(glCreateProgram);
                EXEC_GL(glAttachShader, program, vertex_shader);
                EXEC_GL(glAttachShader, program, fragment_shader);
                EXEC_GL(glLinkProgram, program);

                // Generate a vertex and an index buffer for rectangle vertices
                assert(vertex_buffer == 0);
                EXEC_GL(glGenBuffers, 1, &vertex_buffer);
                assert(index_buffer == 0);
                EXEC_GL(glGenBuffers, 1, &index_buffer);

                // Initialize the index buffer
                static GLushort indices[] = { 0, 1, 3, 2 };
                EXEC_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, index_buffer);
                EXEC_GL(glBufferData, GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLushort), indices, GL_STATIC_DRAW);
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::define_viewport(int x, int y, int width, int height)
            {
                vp_width = width, vp_height = height;
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::prepare_context()
            {
                // TODO: does all this really belong here, or should there be a one-time init independent of viewport ?
                EXEC_GL(glViewport, 0, 0, vp_width, vp_height);
                EXEC_GL(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                EXEC_GL(glEnable, GL_BLEND);
                EXEC_GL(glUseProgram, program);
                gpc::gl::setUniform("vp_width", 0, vp_width);
                gpc::gl::setUniform("vp_height", 1, vp_height);
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::leave_context()
            {
                EXEC_GL(glUseProgram, 0);
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::draw_rect(int x, int y, int w, int h)
            {
                // Prepare the vertices
                GLint v[4][2];
                v[0][0] = x, v[0][1] = y;
                v[1][0] = x, v[1][1] = y + h;
                v[2][0] = x + w, v[2][1] = y + h;
                v[3][0] = x + w, v[3][1] = y;

                // Now send everything to OpenGL
                EXEC_GL(glEnableClientState, GL_VERTEX_ARRAY);
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
                EXEC_GL(glBufferData, GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLint), v, GL_STATIC_DRAW);
                EXEC_GL(glVertexPointer, 2, GL_INT, 2 * sizeof(GLint), nullptr);
                EXEC_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, index_buffer);
                EXEC_GL(glDrawElements, GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr);
                EXEC_GL(glDisableClientState, GL_VERTEX_ARRAY);
                EXEC_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, 0);
            }

            template <bool YAxisDown>
            auto Canvas<YAxisDown>::register_rgba_image(size_t width, size_t height, const RGBA32 *pixels) -> image_handle_t
            {
                auto i = image_textures.size();
                image_textures.resize(i + 1);
                EXEC_GL(glGenTextures, 1, &image_textures[i]);
                EXEC_GL(glBindTexture, GL_TEXTURE_RECTANGLE, image_textures[i]);
                EXEC_GL(glTexImage2D, GL_TEXTURE_RECTANGLE, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                EXEC_GL(glBindTexture, GL_TEXTURE_RECTANGLE, 0);
                return image_textures[i];
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::fill_rect(int x, int y, int w, int h, const native_color_t &color)
            {
                gpc::gl::setUniform("color", 2, color.components);
                gpc::gl::setUniform("render_mode", 5, 1);

                draw_rect(x, y, w, h);
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::draw_image(int x, int y, int w, int h, image_handle_t image)
            {
                draw_image(x, y, w, h, image, 0, 0);
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::draw_image(int x, int y, int w, int h, image_handle_t image, int offset_x, int offset_y)
            {
                static const GLfloat black[4] = { 0, 0, 0, 0 };

                gpc::gl::setUniform("color", 2, black);
                GLint position[2] = { x, y };
                gpc::gl::setUniform("sampler", 3, 0);
                gpc::gl::setUniform("position", 4, position);
                GLint offset[2] = { offset_x, offset_y };
                gpc::gl::setUniform("offset", 6, offset);
                //EXEC_GL(glActiveTexture, GL_TEXTURE0);
                EXEC_GL(glBindTexture, GL_TEXTURE_RECTANGLE, image);
                gpc::gl::setUniform("render_mode", 5, 2);

                draw_rect(x, y, w, h);

                EXEC_GL(glBindTexture, GL_TEXTURE_RECTANGLE, 0);
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::set_clipping_rect(int x, int y, int w, int h)
            {
                EXEC_GL(glScissor, x, YAxisDown ? vp_height - (y + h) : y, w, h);
                EXEC_GL(glEnable, GL_SCISSOR_TEST);
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::cancel_clipping()
            {
                EXEC_GL(glDisable, GL_SCISSOR_TEST);
            }

            // TODO: free resources allocated for fonts
            template <bool YAxisDown>
            auto Canvas<YAxisDown>::register_font(const gpc::fonts::RasterizedFont &rfont) -> font_handle_t
            {
                font_handle_t handle = managed_fonts.size();

                managed_fonts.emplace_back(ManagedFont(rfont));
                auto &mfont = managed_fonts.back();

                mfont.storePixels();
                mfont.createQuads();

                return handle;
            }

            template <bool YAxisDown>
            template <typename CharT>
            void Canvas<YAxisDown>::draw_text(font_handle_t handle, int x, int y, const CharT *text, size_t count)
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
                    setUniform("position", 4, position);

                    GLint base = 4 * glyph_index;
                    EXEC_GL(glDrawArrays, GL_QUADS, base, 4);

                    x += glyph.cbox.adv_x;
                }

                EXEC_GL(glBindTexture, GL_TEXTURE_BUFFER, 0);
                EXEC_GL(glDisableClientState, GL_VERTEX_ARRAY);
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, 0);
            }

            // ManagedFont private class --------------------------------------

            template <bool YAxisDown>
            inline void Canvas<YAxisDown>::ManagedFont::createQuads()
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

                        if (YAxisDown) {
                            /* top left     */ vertices.emplace_back<Vertex>({ glyph.cbox.x_min, -glyph.cbox.y_max });
                            /* bottom left  */ vertices.emplace_back<Vertex>({ glyph.cbox.x_min, -glyph.cbox.y_min });
                            /* bottom right */ vertices.emplace_back<Vertex>({ glyph.cbox.x_max, -glyph.cbox.y_min });
                            /* top right    */ vertices.emplace_back<Vertex>({ glyph.cbox.x_max, -glyph.cbox.y_max });
                        }
                        else {
                            /* bottom left  */ vertices.emplace_back<Vertex>({ glyph.cbox.x_min, glyph.cbox.y_min });
                            /* bottom right */ vertices.emplace_back<Vertex>({ glyph.cbox.x_max, glyph.cbox.y_min });
                            /* top right    */ vertices.emplace_back<Vertex>({ glyph.cbox.x_max, glyph.cbox.y_max });
                            /* top left     */ vertices.emplace_back<Vertex>({ glyph.cbox.x_min, glyph.cbox.y_max });
                        }
                    }
                }

                // Upload the vertex array
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
                EXEC_GL(glBufferData, GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, 0); // just in case
            }

            template <bool YAxisDown>
            void Canvas<YAxisDown>::ManagedFont::storePixels()
            {
                buffer_textures.resize(variants.size());
                EXEC_GL(glGenBuffers, buffer_textures.size(), &buffer_textures[0]);

                textures.resize(variants.size());
                EXEC_GL(glGenTextures, textures.size(), &textures[0]);

                for (auto i_var = 0U; i_var < variants.size(); i_var++) {

                    EXEC_GL(glBindBuffer, GL_TEXTURE_BUFFER, buffer_textures[i_var]);

                    auto &variant = variants[i_var];

                    // Load the pixels into a texture buffer object
                    EXEC_GL(glBufferStorage, GL_TEXTURE_BUFFER, variant.pixels.size(), &variant.pixels[0], 0); // TODO: really no flags ?

                    // Bind the texture buffer object as a.. texture
                    EXEC_GL(glBindTexture, GL_TEXTURE_BUFFER, textures[i_var]);
                    EXEC_GL(glTexBuffer, GL_TEXTURE_BUFFER, GL_R8, buffer_textures[i_var]);
                }

                EXEC_GL(glBindBuffer, GL_TEXTURE_BUFFER, 0);
                EXEC_GL(glBindTexture, GL_TEXTURE_BUFFER, 0);
            }

        } // ns gl
    } // ns gui
} // ns gpc