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

            class Canvas {
            public:

                struct native_color_t { GLfloat components[4]; };
                
                Canvas(): 
                    vertex_buffer(0), index_buffer(0),
                    vertex_shader(0), fragment_shader(0) 
                {
                }

                void init() 
                {
                    static std::once_flag flag;
                    std::call_once(flag, []() { glewInit(); });

                    // Upload and compile our shader program
                    assert(vertex_shader == 0);
                    vertex_shader = CALL_GL(glCreateShader, GL_FRAGMENT_SHADER);

                    // Generate a vertex and an index buffer for rectangle vertices
                    assert(vertex_buffer == 0);
                    EXEC_GL(glGenBuffers, 1, &vertex_buffer);
                    assert(index_buffer == 0);
                    EXEC_GL(glGenBuffers, 1, &index_buffer);

                    // Initialize the index buffer
                    static GLushort indices[] = { 0, 1, 3, 2 };
                    EXEC_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, index_buffer);
                    EXEC_GL(glBufferData, GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLushort), indices, GL_STATIC_DRAW);

                    // TODO...
                }

                auto rgb_to_native(const RGBF &color) -> native_color_t {
                    return native_color_t { { color.r, color.g, color.b, 1 } };
                }
                auto rgba_to_native(const RGBAF &color) -> native_color_t {
                    return native_color_t{ { color.r, color.g, color.b, color.a } };
                }

                // TODO: top-left vs. bottom-left
                void fill_rect(int x, int y, int w, int h, const native_color_t &color) 
                {
                    GLint v[4][2];
                    v[0][0] = x    , v[0][1] = y;
                    v[1][0] = x    , v[1][1] = y + h;
                    v[2][0] = x + w, v[2][1] = y + h;
                    v[3][0] = x + w, v[3][1] = y;
                    EXEC_GL(glEnableClientState, GL_VERTEX_ARRAY);
                    EXEC_GL(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer);
                    EXEC_GL(glBufferData, GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLint), v, GL_STATIC_DRAW);
                    EXEC_GL(glVertexPointer, 2, GL_INT, 2 * sizeof(GLint), nullptr);
                    EXEC_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, index_buffer);
                    EXEC_GL(glDrawElements, GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr);
                    EXEC_GL(glDisableClientState, GL_VERTEX_ARRAY);
                }

            private:

                static std::string vertex_code;

                GLuint vertex_buffer, index_buffer;
                GLuint vertex_shader, fragment_shader;
            };

        } // ns gl
    } // ns gui
} // ns gpc