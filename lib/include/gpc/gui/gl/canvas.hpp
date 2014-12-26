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


        namespace gl {

            class _CanvasBase {
            public:

                struct native_color_t { GLfloat components[4]; };

                typedef GLuint image_handle_t;

                auto rgb_to_native(const RGBFloat &color) -> native_color_t {
                    return native_color_t{ { color.r, color.g, color.b, 1 } };
                }
                auto rgba_to_native(const RGBAFloat &color) -> native_color_t {
                    return native_color_t{ { color.r, color.g, color.b, color.a } };
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

            protected:

                _CanvasBase();

                void init(bool y_axis_downward);

                //void draw_rect(const GLint *v);

                void draw_rect(int x, int y, int width, int height);

            protected:

                static const std::string vertex_code, fragment_code;

                GLuint vertex_buffer, index_buffer;
                GLuint vertex_shader, fragment_shader;
                GLuint program;
                std::vector<GLuint> textures;
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

        } // ns gl
    } // ns gui
} // ns gpc