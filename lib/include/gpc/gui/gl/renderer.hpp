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

#include <gpc/gl/wrappers.hpp>
#include <gpc/gl/uniform.hpp>
#include <gpc/gl/shader_program.hpp>
#include <gpc/fonts/rasterized_font.hpp>
#include <gpc/gui/renderer.hpp>

namespace gpc {

    namespace gui {

        // TODO: is a dedicated namespace really necessary ?

        namespace gl {

            // TODO: this will fail if there is no such namespace (e.g. when using GLEW)
            using namespace ::gl;

            /** This templated class implements a yet to be defined compile-time interface (concept)
                that would probably best be called something like "PixelGridFittingRenderer" and 
                which is a specialization of "Renderer", both of which would belong to the gpc::gui
                namespace.

                TODO: inherit from base Renderer that defines default metadata (see below)
                TODO: provide a concept checker ?
                TODO: change "rectangle" interfaces to use vertices only instead of vertices + extents ?
             */
            template <
                bool YAxisDown
            >
            class renderer {
            public:

                // Metadata

                static const bool font_mapping_is_expensive = true;
                static const bool color_mapping_is_expensive = false;
                //static const bool font_handles_are_resources = true;
                //static const bool colors_are_resources       = false;

            public:

                using rasterized_font = gpc::fonts::rasterized_font;

            public:

                // Exported types

                using offset        = int;
                using length        = int;
                using image_handle  = GLuint;
                using font_handle   = GLint;
                using native_color  = rgba_norm;
                using native_mono   = mono_norm;

                // Class methods

                static constexpr auto rgba_to_native(const rgba_norm &color) -> rgba_norm
                {
                    return color;
                }

                static constexpr auto mono_to_native(const mono_norm &color) -> mono_norm
                {
                    return color;
                }

                // Lifecycle

                renderer();

                void enter_context();

                void leave_context();

                void define_viewport(int x, int y, int width, int height);

                void clear(const rgba_norm &color);

                auto register_rgba32_image(size_t width, size_t height, const rgba32 *pixels) -> image_handle;

                void release_rgba32_image(image_handle);

                auto register_mono8_image(size_t width, size_t height, const mono8 *pixels) -> image_handle; 
                    // TODO: use image handle type specific to mono images?

                void release_mono8_image(image_handle);

                void fill_rect(int x, int y, int w, int h, const rgba_norm &color);

                // TODO: deprecate and rename to draw_color_image()
                void draw_image(int x, int y, int w, int h, image_handle image);

                // TODO: deprecate and rename to draw_color_image()
                void draw_image(int x, int y, int w, int h, image_handle image, int offset_x, int offset_y);

                void modulate_greyscale_image(int x, int y, int w, int h, image_handle, const rgba_norm &color, 
                    int offset_x = 0, int offset_y = 0);

                /** Collection of image drawing methods intended for drawing an image in the
                    4 cardinal directions.
                */

                void draw_greyscale_image_right_righthand(int x, int y, int length, int width, 
                    image_handle, const rgba_norm &color,  int offset_x = 0, int offset_y = 0);

                void draw_greyscale_image_down_righthand(int x, int y, int length, int width, 
                    image_handle, const rgba_norm &color,  int offset_x = 0, int offset_y = 0);

                void draw_greyscale_image_left_righthand(int x, int y, int length, int width, 
                    image_handle, const rgba_norm &color,  int offset_x = 0, int offset_y = 0);

                void draw_greyscale_image_up_righthand(int x, int y, int length, int width, 
                    image_handle, const rgba_norm &color,  int offset_x = 0, int offset_y = 0);

                void set_clipping_rect(int x, int y, int w, int h);

                void cancel_clipping();

                auto register_font(const rasterized_font &font) -> font_handle;

                void release_font(font_handle reg_font);
                //void release_font(const rasterized_font &);

                void set_text_color(const rgba_norm &color);

                // auto get_text_extents(reg_font_t font, const char32_t *text, size_t count) -> text_bbox_t;

                void render_text(font_handle font, int x, int y, const char32_t *text, size_t count, int w_max = 0);

                void init();

                void cleanup();

                void draw_rect(int x, int y, int width, int height);

            private:

                void _draw_greyscale_image(int x, int y, int w, int h, image_handle, const rgba_norm &color,
                    int origin_x, int origin_y, float texrot_sin, float texrot_cos, int offset_x = 0, int offset_y = 0);

                // TODO: move this back into non-template base class

                static constexpr auto vertex_code() -> std::string {

                    return std::string {
                        #include "gpc/gui/gl/vertex.glsl.h"
                    };
                }

                static constexpr auto fragment_code() -> std::string {

                    return std::string {
                        #include "gpc/gui/gl/fragment.glsl.h"
                    };
                }

                struct managed_font: gpc::fonts::rasterized_font {
                    
                    managed_font(const rasterized_font &font_) : gpc::fonts::rasterized_font{ font_ } {}
                    
                    void store_pixels();
                    void create_quads();

                    std::vector<GLuint> buffer_textures;
                    std::vector<GLuint> textures; // one 1D texture per variant
                    GLuint vertex_buffer;
                };

                //static const std::string vertex_code, fragment_code;

                GLuint vertex_buffer, index_buffer;
                GLuint vertex_shader, fragment_shader;
                GLuint program;
                std::vector<GLuint> image_textures;
                std::vector<managed_font> managed_fonts;
                GLint vp_width, vp_height;
                rgba_norm text_color;

                #ifdef DEBUG
                bool dbg_clipping_active = false;
                #endif
            };

            // Method implementations -----------------------------------------

            template <bool YAxisDown>
            renderer<YAxisDown>::renderer() :
                vertex_buffer(0), index_buffer(0),
                vertex_shader(0), fragment_shader(0), program(0)
            {
                text_color = rgba_to_native({0, 0, 0, 1});
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::init()
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
                    vertex_shader = GL(CreateShader, GL_VERTEX_SHADER);
                    auto code = vertex_code();
                    if (YAxisDown) code = gpc::gl::insertLinesIntoShaderSource(code, "#define Y_AXIS_DOWN");
                    // TODO: dispense with the error checking and logging in release builds
                    auto log = ::gpc::gl::compileShader(vertex_shader, code);
                    if (!log.empty()) std::cerr << "Vertex shader compilation log:" << std::endl << log << std::endl;
                }
                {
                    assert(fragment_shader == 0);
                    fragment_shader = GL(CreateShader, GL_FRAGMENT_SHADER);
                    auto code = fragment_code();
                    if (YAxisDown) code = gpc::gl::insertLinesIntoShaderSource(code, "#define Y_AXIS_DOWN");
                    //std::cerr << code << std::endl;
                    // TODO: dispense with the error checking and logging in release builds
                    auto log = gpc::gl::compileShader(fragment_shader, code);
                    if (!log.empty()) std::cerr << "Fragment shader compilation log:" << std::endl << log << std::endl;
                }
                assert(program == 0);
                program = GL(CreateProgram);
                GL(AttachShader, program, vertex_shader);
                GL(AttachShader, program, fragment_shader);
                GL(LinkProgram, program);
                //GL(ValidateProgram, program);
                char log[2048];
                GLsizei len;
                GL(GetProgramInfoLog, program, 2048, &len, log);
                if (log[0] != '\0') {
                    std::cerr << "Shader info log:" << std::endl << log << std::endl;
                    throw std::runtime_error("gpc::gui::gl::renderer: failed to build shader program");
                }

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
            void renderer<YAxisDown>::cleanup()
            {
                // TODO: free all resources
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::define_viewport(int x, int y, int w, int h)
            {
                vp_width = w, vp_height = h;
                GL(Viewport, x, y, w, h);

                GL(UseProgram, program);
                ::gpc::gl::setUniform("viewport_w", 0, w);
                ::gpc::gl::setUniform("viewport_h", 1, h);
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::enter_context()
            {
                // TODO: does all this really belong here, or should there be a one-time init independent of viewport ?
                GL(BlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                GL(Enable, GL_BLEND);
                GL(Disable, GL_DEPTH_TEST);
                GL(UseProgram, program);
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::leave_context()
            {
                GL(UseProgram, 0);
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::clear(const rgba_norm &color)
            {
                GL(ClearColor, color.r(), color.g(), color.b(), color.a());
                GL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::draw_rect(int x, int y, int w, int h)
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
                GL(BindBuffer, GL_ARRAY_BUFFER, 0);
            }

            template <bool YAxisDown>
            auto renderer<YAxisDown>::register_rgba32_image(size_t width, size_t height, const rgba32 *pixels) -> image_handle
            {
                auto i = image_textures.size();
                image_textures.resize(i + 1);
                GL(GenTextures, 1, &image_textures[i]);
                //GL(ActiveTexture, GL_TEXTURE0);
                GL(BindTexture, GL_TEXTURE_RECTANGLE, image_textures[i]);
                GL(TexImage2D, GL_TEXTURE_RECTANGLE, 0, (GLint)GL_RGBA, width, height, 0, (GLenum)GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                GL(BindTexture, GL_TEXTURE_RECTANGLE, 0);
                return image_textures[i];
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::release_rgba32_image(image_handle hnd)
            {
                auto i = std::find(std::begin(image_textures), std::end(image_textures), hnd);
                assert(i != std::end(image_textures));
                GL(DeleteTextures, 1, &hnd);
                *i = 0; // TODO: put into "recycle" list ?
            }

            template<bool YAxisDown>
            inline auto renderer<YAxisDown>::register_mono8_image(size_t width, size_t height, const mono8 *pixels) -> image_handle
            {
                auto i = image_textures.size();
                image_textures.resize(i + 1);
                GL(GenTextures, 1, &image_textures[i]);
                //GL(ActiveTexture, GL_TEXTURE0);
                GL(BindTexture, GL_TEXTURE_RECTANGLE, image_textures[i]);
                GL(TexImage2D, GL_TEXTURE_RECTANGLE, 0, (GLint)GL_ALPHA, width, height, 0, (GLenum)GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
                GL(BindTexture, GL_TEXTURE_RECTANGLE, 0);
                return image_textures[i];
            }

            template<bool YAxisDown>
            inline void renderer<YAxisDown>::release_mono8_image(image_handle hnd)
            {
                release_rgba32_image(hnd); // same resource list
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::fill_rect(int x, int y, int w, int h, const rgba_norm &color)
            {
                GL(Uniform4fv, 2, 1, color);
                gpc::gl::setUniform("render_mode", 5, 1);

                draw_rect(x, y, w, h);
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::draw_image(int x, int y, int w, int h, image_handle image)
            {
                draw_image(x, y, w, h, image, 0, 0);
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::draw_image(int x, int y, int w, int h, image_handle image, int offset_x, int offset_y)
            {
                static const GLfloat black[4] = { 0, 0, 0, 0 };

                //GL(ActiveTexture, GL_TEXTURE0);
                GL(BindTexture, GL_TEXTURE_RECTANGLE, image);
                gpc::gl::setUniform("color", 2, black);
                GLint position[2] = { x, y };
                gpc::gl::setUniform("sampler", 3, 0);
                gpc::gl::setUniform("position", 4, position);
                GLint offset[2] = { offset_x, offset_y };
                gpc::gl::setUniform("offset", 6, offset);
                gpc::gl::setUniform("render_mode", 5, 2); // 2 = "paste image"

                draw_rect(x, y, w, h);

                GL(BindTexture, GL_TEXTURE_RECTANGLE, 0);
            }

            // TODO: rename to "modulate_greyscale_image()" ?
            template<bool YAxisDown>
            inline void renderer<YAxisDown>::modulate_greyscale_image(int x, int y, int w, int h, 
                image_handle img, const rgba_norm &color, int offset_x, int offset_y)
            {
                _draw_greyscale_image(x, y, w, h, img, color, 0, 0, 0, 1, offset_x, offset_y);
            }

            template<bool YAxisDown>
            inline void renderer<YAxisDown>::draw_greyscale_image_right_righthand(int x, int y, int length, int width, 
                image_handle img, const rgba_norm &color, int offset_x, int offset_y)
            {
                _draw_greyscale_image(x, y, length, width, img, color, 
                    0, 0, // origin is on top left vertex
                    0, 1, // don't rotate texture
                    offset_x, offset_y
                    );
            }

            template<bool YAxisDown>
            inline void renderer<YAxisDown>::draw_greyscale_image_down_righthand(int x, int y, int length, int width, 
                image_handle img, const rgba_norm &color, int offset_x, int offset_y)
            {
                _draw_greyscale_image(x - width, y, width, length, img, color, 
                    length, 0, // origin is on top right vertex
                    1, 0, // rotate texture 90° clockwise (sin theta = 1, cos theta = 0)
                    offset_x, offset_y
                    );
            }

            template<bool YAxisDown>
            inline void renderer<YAxisDown>::draw_greyscale_image_left_righthand(int x, int y, int length, int width, 
                image_handle img, const rgba_norm &color, int offset_x, int offset_y)
            {
                _draw_greyscale_image(x - length, y - width, length, width, img, color, 
                    length, 1, // origin is on bottom right vertex
                    0, -1, // rotate texture 180° clockwise (sin theta = 0, cos theta = -1)
                    offset_x, offset_y
                    );
            }

            template<bool YAxisDown>
            inline void renderer<YAxisDown>::draw_greyscale_image_up_righthand(int x, int y, int length, int width, 
                image_handle img, const rgba_norm &color, int offset_x, int offset_y)
            {
                _draw_greyscale_image(x, y - length, width, length, img, color, 
                    0, length, // origin is on bottom left vertex
                    -1, 0, // rotate texture 270° clockwise (sin theta = -1, cos theta = 0)
                    offset_x, offset_y
                    );
            }

            template<bool YAxisDown>
            inline void renderer<YAxisDown>::_draw_greyscale_image(int x, int y, int w, int h, image_handle img, const rgba_norm &color, 
                int origin_x, int origin_y, float texrot_sin, float texrot_cos, int offset_x, int offset_y)
            {
                using namespace gpc::gl;

                auto native_clr = rgba_to_native(color);

                //GL(ActiveTexture, GL_TEXTURE0);
                GL(BindTexture, GL_TEXTURE_RECTANGLE, img);
                setUniform("color", 2, native_clr.components);
                GLint position[2] = { x + origin_x, y + origin_y };
                setUniform("sampler", 3, 0);
                setUniform("position", 4, position);
                GLint offset[2] = { offset_x, offset_y };
                setUniform("offset", 6, offset);
                GLfloat texcoord_matrix[2][2] = { texrot_cos, - texrot_sin, texrot_sin, texrot_cos };
                setUniformMatrix2("texcoord_matrix", 10, &texcoord_matrix[0][0]);
                setUniform("render_mode", 5, 4); // 4 = "modulate greyscale image"

                draw_rect(x, y, w, h);

                GL(BindTexture, GL_TEXTURE_RECTANGLE, 0);
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::set_clipping_rect(int x, int y, int w, int h)
            {
                #ifdef DEBUG
                assert(!dbg_clipping_active);
                #endif

                GL(Scissor, x, YAxisDown ? vp_height - (y + h) : y, w, h);
                GL(Enable, GL_SCISSOR_TEST);

                #ifdef DEBUG
                dbg_clipping_active = true;
                #endif
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::cancel_clipping()
            {
                #ifdef DEBUG
                assert(dbg_clipping_active);
                dbg_clipping_active = false;
                #endif

                GL(Disable, GL_SCISSOR_TEST);
            }

            // TODO: free resources allocated for fonts
            template <bool YAxisDown>
            auto renderer<YAxisDown>::register_font(const gpc::fonts::rasterized_font &font) -> font_handle
            {
                // TODO: re-use discarded slots
                font_handle index = managed_fonts.size();

                managed_fonts.emplace_back(managed_font{ font });
                auto &mf = managed_fonts.back();

                mf.store_pixels();
                mf.create_quads();

                return index + 1;
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::release_font(font_handle /*handle*/)
            {
                // auto &font = managed_fonts[handle - 1];
                // TODO: actual implementation
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::set_text_color(const rgba_norm &color)
            {
                text_color = color;
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::render_text(font_handle handle, int x, int y, const char32_t *text, size_t count, int w_max)
            {
                // TODO: support text that advances in Y direction (and right-to-left)

                using gpc::gl::setUniform;

                const auto &mfont = managed_fonts[handle - 1];

                auto var_index = 0; // TODO: support multiple variants
                const auto &variant = mfont.variants[var_index]; 

                GL(EnableClientState, GL_VERTEX_ARRAY);
                GL(BindBuffer, GL_ARRAY_BUFFER, mfont.vertex_buffer);
                GL(VertexPointer, 2, GL_INT, 0, static_cast<GLvoid*>(0));

                GL(BindTexture, GL_TEXTURE_BUFFER, mfont.textures[var_index]); // font pixels

                int dx = 0;

                auto glyph_index = mfont.find_glyph(*text);
                auto glyph = & variant.glyphs[glyph_index];
                dx -= glyph->cbox.bounds.x_min;

                GL(Uniform4fv, 2, 1, text_color); // 2 = color
                setUniform("render_mode", 5, 3);
                setUniform("font_pixels", 7, 0); // use texture unit 0 to access glyph pixels

                for (const auto *p = text; p < (text + count); p++)
                {
                    glyph_index = mfont.find_glyph(*p);
                    glyph = & variant.glyphs[glyph_index];

                    setUniform("glyph_base", 8, glyph->pixel_base);
                    GLint cbox[4] = { 
                        glyph->cbox.bounds.x_min, glyph->cbox.bounds.x_max, 
                        glyph->cbox.bounds.y_min, glyph->cbox.bounds.y_max };
                    setUniform("glyph_cbox", 9, cbox);
                    GLint position[2] = { x + dx, y };
                    setUniform("position", 4, position);

                    GLint base = 4 * glyph_index;
                    GL(DrawArrays, GL_QUADS, base, 4);

                    dx += glyph->cbox.adv_x;

                    if (w_max > 0 && dx >= w_max) break;
                }

                GL(BindTexture, GL_TEXTURE_BUFFER, 0);
                GL(DisableClientState, GL_VERTEX_ARRAY);
                GL(BindBuffer, GL_ARRAY_BUFFER, 0);
            }

            // managed_font private class -------------------------------------

            template <bool YAxisDown>
            inline void renderer<YAxisDown>::managed_font::create_quads()
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

#ifndef NOT_DEFINED
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
#else
                        /* bottom left  */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_min,  glyph.cbox.bounds.y_min });
                        /* bottom right */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_max,  glyph.cbox.bounds.y_min });
                        /* top right    */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_max,  glyph.cbox.bounds.y_max });
                        /* top left     */ vertices.emplace_back<Vertex>({ glyph.cbox.bounds.x_min,  glyph.cbox.bounds.y_max });
#endif
                    }
                }

                // Upload the vertex array
                GL(BindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
                GL(BufferData, GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
                GL(BindBuffer, GL_ARRAY_BUFFER, 0); // just in case
            }

            template <bool YAxisDown>
            void renderer<YAxisDown>::managed_font::store_pixels()
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