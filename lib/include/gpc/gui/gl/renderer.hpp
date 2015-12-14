#pragma once

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <string>
#include <array>
#ifdef FORCE_GLEW
#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/glew.h>
#endif

#include <gpc/gui/renderer.hpp>
#include <gpc/gl/wrappers.hpp>
#include <gpc/gl/uniform.hpp>
#include <gpc/gl/shader_program.hpp>
#include <gpc/fonts/rasterized_font.hpp>

namespace gpc {

    namespace gui {

        // TODO: is a dedicated namespace really necessary ?

        namespace gl {

            /** This templated class implements a yet to be defined compile-time interface (concept)
                that would probably best be called something like "PixelGridFittingRenderer" and 
                which is a specialization of "Renderer", both of which would belong to the gpc::gui
                namespace.
             */
            template <
                bool YAxisDown
            >
            class Renderer {
            public:

                typedef int offset_t;
                typedef int length_t;

                struct native_color_t { 
                    GLclampf components[4]; 
                    GLclampf r() const { return components[0]; }
                    GLclampf g() const { return components[1]; }
                    GLclampf b() const { return components[2]; }
                    GLclampf a() const { return components[3]; }
                };

                typedef GLuint image_handle_t;

                typedef GLint reg_font_t;

                Renderer();

                // TODO: define color conversions as instance-less ("static") ? or even constexpr ?
                static auto rgb_to_native(const RGBFloat &color) -> native_color_t {
                    return native_color_t { { color.r, color.g, color.b, 1 } };
                }
                static auto rgba_to_native(const RGBAFloat &color) -> native_color_t {
                    return native_color_t { { color.r, color.g, color.b, color.a } };
                }

                void enter_context();

                void leave_context();

                void define_viewport(int x, int y, int width, int height);

                void clear(const native_color_t &color);

                auto register_rgba_image(size_t width, size_t height, const RGBA32 *pixels) -> image_handle_t;

                void fill_rect(int x, int y, int w, int h, const native_color_t &color);

                void draw_image(int x, int y, int w, int h, image_handle_t image);

                void draw_image(int x, int y, int w, int h, image_handle_t image, int offset_x, int offset_y);

                void set_clipping_rect(int x, int y, int w, int h);

                void cancel_clipping();

                auto register_font(const gpc::fonts::rasterized_font &rfont) -> reg_font_t;

                void release_font(reg_font_t reg_font);

                void set_text_color(const native_color_t &color);

                // auto get_text_extents(reg_font_t font, const char32_t *text, size_t count) -> text_bbox_t;

                void render_text(reg_font_t font, int x, int y, const char32_t *text, size_t count);

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

                struct managed_font: gpc::fonts::rasterized_font {
                    managed_font(const gpc::fonts::rasterized_font &from): gpc::fonts::rasterized_font(from) {}
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
                std::vector<managed_font> managed_fonts;
                GLint vp_width, vp_height;
                native_color_t text_color;
            };

            // Method implementations -----------------------------------------

            template <bool YAxisDown>
            Renderer<YAxisDown>::Renderer() :
                vertex_buffer(0), index_buffer(0),
                vertex_shader(0), fragment_shader(0), program(0)
            {
                text_color = rgba_to_native({0, 0, 0, 1});
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::init()
            {
                // User code is responsible for creating and/or selecting the proper GL context when calling init()
                // TODO: somehow (optionally) make use of glbinding's context management facilities?
                #ifdef NOT_DEFINED
                static std::once_flag flag;
                std::call_once(flag, []() { glewInit(); });
                #endif

                // Upload and compile our shader program
                {
                    assert(vertex_shader == 0);
                    vertex_shader = CALL_GL(glCreateShader, GL_VERTEX_SHADER);
                    // TODO: dispense with the error checking and logging in release builds
                    auto log = ::gpc::gl::compileShader(vertex_shader, vertex_code(), YAxisDown ? "#define Y_AXIS_DOWN" : "");
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
                program = GL(CreateProgram);
                GL(AttachShader, program, vertex_shader);
                GL(AttachShader, program, fragment_shader);
                GL(LinkProgram, program);

                // Generate a vertex and an index buffer for rectangle vertices
                assert(vertex_buffer == 0);
                GL(GenBuffers, 1, &vertex_buffer);
                assert(index_buffer == 0);
                GL(GenBuffers, 1, &index_buffer);

                // Initialize the index buffer
                static GLushort indices[] = { 0, 1, 3, 2 };
                GL(BindBuffer, GL_ELEMENT_ARRAY_BUFFER, index_buffer);
                GL(BufferData, GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLushort), indices, GL_STATIC_DRAW);
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::define_viewport(int x, int y, int w, int h)
            {
                vp_width = w, vp_height = h;
                //GL(Viewport, x, y, w, h);

                GL(UseProgram, program);
                ::gpc::gl::setUniform("vp_width", 0, w);
                ::gpc::gl::setUniform("vp_height", 1, h);
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::enter_context()
            {
                // TODO: does all this really belong here, or should there be a one-time init independent of viewport ?
                GL(BlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                GL(Enable, GL_BLEND);
                GL(Disable, GL_DEPTH_TEST);
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::leave_context()
            {
                GL(UseProgram, 0);
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::clear(const native_color_t &color)
            {
                GL(ClearColor, color.r(), color.g(), color.b(), color.a());
                GL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::draw_rect(int x, int y, int w, int h)
            {
                // Prepare the vertices
                GLint v[4][2];
                v[0][0] = x, v[0][1] = y;
                v[1][0] = x, v[1][1] = y + h;
                v[2][0] = x + w, v[2][1] = y + h;
                v[3][0] = x + w, v[3][1] = y;

                // Now send everything to OpenGL
                GL(EnableClientState, GL_VERTEX_ARRAY);
                GL(BindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
                GL(BufferData, GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLint), v, GL_STATIC_DRAW);
                GL(VertexPointer, 2, GL_INT, 2 * sizeof(GLint), nullptr);
                GL(BindBuffer, GL_ELEMENT_ARRAY_BUFFER, index_buffer);
                GL(DrawElements, GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr);
                GL(DisableClientState, GL_VERTEX_ARRAY);
                GL(BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
                EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, 0);
            }

            template <bool YAxisDown>
            auto Renderer<YAxisDown>::register_rgba_image(size_t width, size_t height, const RGBA32 *pixels) -> image_handle_t
            {
                auto i = image_textures.size();
                image_textures.resize(i + 1);
                GL(GenTextures, 1, &image_textures[i]);
                GL(BindTexture, GL_TEXTURE_RECTANGLE, image_textures[i]);
                GL(TexImage2D, GL_TEXTURE_RECTANGLE, 0, (GLint)GL_RGBA, width, height, 0, (GLenum)GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                GL(BindTexture, GL_TEXTURE_RECTANGLE, 0);
                return image_textures[i];
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::fill_rect(int x, int y, int w, int h, const native_color_t &color)
            {
                gpc::gl::setUniform("color", 2, color.components);
                gpc::gl::setUniform("render_mode", 5, 1);

                draw_rect(x, y, w, h);
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::draw_image(int x, int y, int w, int h, image_handle_t image)
            {
                draw_image(x, y, w, h, image, 0, 0);
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::draw_image(int x, int y, int w, int h, image_handle_t image, int offset_x, int offset_y)
            {
                static const GLfloat black[4] = { 0, 0, 0, 0 };

                gpc::gl::setUniform("color", 2, black);
                GLint position[2] = { x, y };
                gpc::gl::setUniform("sampler", 3, 0);
                gpc::gl::setUniform("position", 4, position);
                GLint offset[2] = { offset_x, offset_y };
                gpc::gl::setUniform("offset", 6, offset);
                //GL(ActiveTexture, GL_TEXTURE0);
                GL(BindTexture, GL_TEXTURE_RECTANGLE, image);
                gpc::gl::setUniform("render_mode", 5, 2);

                draw_rect(x, y, w, h);

                GL(BindTexture, GL_TEXTURE_RECTANGLE, 0);
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::set_clipping_rect(int x, int y, int w, int h)
            {
                GL(Scissor, x, YAxisDown ? vp_height - (y + h) : y, w, h);
                GL(Enable, GL_SCISSOR_TEST);
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::cancel_clipping()
            {
                GL(Disable, GL_SCISSOR_TEST);
            }

            // TODO: free resources allocated for fonts
            template <bool YAxisDown>
            auto Renderer<YAxisDown>::register_font(const gpc::fonts::rasterized_font &rfont) -> reg_font_t
            {
                // TODO: re-use discarded slots
                reg_font_t index = managed_fonts.size();

                managed_fonts.emplace_back(managed_font(rfont));
                auto &mfont = managed_fonts.back();

                mfont.storePixels();
                mfont.createQuads();

                return index + 1;
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::release_font(reg_font_t handle)
            {
                auto &font = managed_fonts[handle - 1];
                // TODO: actual implementation
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::set_text_color(const native_color_t &color)
            {
                text_color = color;
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::render_text(reg_font_t handle, int x, int y, const char32_t *text, size_t count)
            {
                using gpc::gl::setUniform;

                const auto &mfont = managed_fonts[handle - 1];

                auto var_index = 0; // TODO: support multiple variants
                const auto &variant = mfont.variants[var_index]; 

                GL(EnableClientState, GL_VERTEX_ARRAY);
                GL(BindBuffer, GL_ARRAY_BUFFER, mfont.vertex_buffer);
                GL(VertexPointer, 2, GL_INT, 0, static_cast<GLvoid*>(0));

                GL(BindTexture, GL_TEXTURE_BUFFER, mfont.textures[var_index]); // font pixels

                {
                    auto glyph_index = mfont.find_glyph(*text);
                    const auto &glyph = variant.glyphs[glyph_index];
                    x -= glyph.cbox.bounds.x_min;
                }

                setUniform("color", 2, text_color.components);
                setUniform("render_mode", 5, 3);
                setUniform("font_pixels", 7, 0); // use texture unit 0 to access glyph pixels

                for (const auto *p = text; p < (text + count); p++) {

                    auto glyph_index = mfont.find_glyph(*p);
                    const auto &glyph = variant.glyphs[glyph_index];

                    setUniform("glyph_base", 8, glyph.pixel_base);
                    GLint cbox[4] = { 
                        glyph.cbox.bounds.x_min, glyph.cbox.bounds.x_max, 
                        glyph.cbox.bounds.y_min, glyph.cbox.bounds.y_max };
                    setUniform("glyph_cbox", 9, cbox);
                    GLint position[2] = { x, y };
                    setUniform("position", 4, position);

                    GLint base = 4 * glyph_index;
                    GL(DrawArrays, GL_QUADS, base, 4);

                    x += glyph.cbox.adv_x;
                }

                GL(BindTexture, GL_TEXTURE_BUFFER, 0);
                GL(DisableClientState, GL_VERTEX_ARRAY);
                GL(BindBuffer, GL_ARRAY_BUFFER, 0);
            }

            // managed_font private class -------------------------------------

            template <bool YAxisDown>
            inline void Renderer<YAxisDown>::managed_font::createQuads()
            {
                struct Vertex { GLint x, y; };

                GL(GenBuffers, 1, &vertex_buffer);

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
                            /* top left     */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_min, -glyph.cbox.bounds.y_max });
                            /* bottom left  */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_min, -glyph.cbox.bounds.y_min });
                            /* bottom right */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_max, -glyph.cbox.bounds.y_min });
                            /* top right    */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_max, -glyph.cbox.bounds.y_max });
                        }                                                               
                        else {                                                          
                            /* bottom left  */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_min,  glyph.cbox.bounds.y_min });
                            /* bottom right */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_max,  glyph.cbox.bounds.y_min });
                            /* top right    */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_max,  glyph.cbox.bounds.y_max });
                            /* top left     */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_min,  glyph.cbox.bounds.y_max });
                        }
                    }
                }

                // Upload the vertex array
                GL(BindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
                GL(BufferData, GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
                GL(BindBuffer, GL_ARRAY_BUFFER, 0); // just in case
            }

            template <bool YAxisDown>
            void Renderer<YAxisDown>::managed_font::storePixels()
            {
                buffer_textures.resize(variants.size());
                GL(GenBuffers, buffer_textures.size(), &buffer_textures[0]);

                textures.resize(variants.size());
                GL(GenTextures, textures.size(), &textures[0]);

                for (auto i_var = 0U; i_var < variants.size(); i_var++) {

                    GL(BindBuffer, GL_TEXTURE_BUFFER, buffer_textures[i_var]);

                    auto &variant = variants[i_var];

                    // Load the pixels into a texture buffer object
                    // TODO: really no flags ?
                    GL(BufferStorage, GL_TEXTURE_BUFFER, variant.pixels.size(), &variant.pixels[0], (BufferStorageMask)0);

                    // Bind the texture buffer object as a.. texture
                    GL(BindTexture, GL_TEXTURE_BUFFER, textures[i_var]);
                    GL(TexBuffer, GL_TEXTURE_BUFFER, GL_R8, buffer_textures[i_var]);
                }

                GL(BindBuffer, GL_TEXTURE_BUFFER, 0);
                GL(BindTexture, GL_TEXTURE_BUFFER, 0);
            }

        } // ns gl
    } // ns gui
} // ns gpc