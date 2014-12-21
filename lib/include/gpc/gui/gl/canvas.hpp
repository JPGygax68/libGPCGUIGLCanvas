#pragma once

#include <cassert>
#include <thread>
#include <mutex>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/glew.h>

#include <gpc/gl/wrappers.hpp>

namespace gpc {

    namespace gui {

        // TODO: RGBF and RGBAF are implementation-independent and belong in
        // the module defining the Painter concept

        struct RGBF {
            GLfloat r, g, b;
        };

        struct RGBAF {
            GLfloat r, g, b, a;
        };

        namespace gl {

            class _CanvasBase {
            public:

                struct native_color_t { GLfloat components[4]; };

                auto rgb_to_native(const RGBF &color) -> native_color_t {
                    return native_color_t{ { color.r, color.g, color.b, 1 } };
                }
                auto rgba_to_native(const RGBAF &color) -> native_color_t {
                    return native_color_t{ { color.r, color.g, color.b, color.a } };
                }

                void define_viewport(int x, int y, int width, int height);

            protected:

                _CanvasBase();

                void init();

                void prepare_context();

                void leave_context();

                void draw_rect(const GLint *v);

            protected:

                static std::string vertex_code, fragment_code;

                GLuint vertex_buffer, index_buffer;
                GLuint vertex_shader, fragment_shader;
                GLuint program;
                GLint colour_location;
                GLint vp_width, vp_height;
                GLint vp_width_location, vp_height_location;
            };

            class Canvas : public _CanvasBase {
            public:

                Canvas(): _CanvasBase() {}

                void init() 
                {
                    _CanvasBase::init();
                }

                void prepare_context()
                {
                    _CanvasBase::prepare_context();
                }

                void leave_context()
                {
                    _CanvasBase::leave_context();
                }

                // TODO: top-left vs. bottom-left
                void fill_rect(int x, int y, int w, int h, const native_color_t &color) 
                {
                    GLint v[4][2];
                    v[0][0] = x    , v[0][1] = y;
                    v[1][0] = x    , v[1][1] = y + h;
                    v[2][0] = x + w, v[2][1] = y + h;
                    v[3][0] = x + w, v[3][1] = y;

                    EXEC_GL(glUniform4fv, colour_location, 1, color.components);

                    draw_rect(&v[0][0]);
                }

            private:

            };

        } // ns gl
    } // ns gui
} // ns gpc