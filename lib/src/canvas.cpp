#include <string>
#include <cassert>

#include <gpc/gui/gl/canvas.hpp>

namespace gpc { 

    namespace gui { 

        namespace gl {

            const std::string _CanvasBase::vertex_code {
                #include "vertex.glsl.h"
            };

            const std::string _CanvasBase::fragment_code{
                #include "fragment.glsl.h"
            };

            _CanvasBase::_CanvasBase() :
                vertex_buffer(0), index_buffer(0),
                vertex_shader(0), fragment_shader(0), program(0)
            {
            }

            void _CanvasBase::init(bool y_axis_downward)
            {
                static std::once_flag flag;
                std::call_once(flag, []() { glewInit(); });

                // Upload and compile our shader program
                {
                    assert(vertex_shader == 0);
                    vertex_shader = CALL_GL(glCreateShader, GL_VERTEX_SHADER);
                    auto log = gpc::gl::compileShader(vertex_shader, vertex_code, y_axis_downward ? "#define Y_AXIS_DOWN" : "");
                    if (!log.empty()) std::cerr << "Vertex shader compilation log:" << std::endl << log << std::endl;
                }
                {
                    assert(fragment_shader == 0);
                    fragment_shader = CALL_GL(glCreateShader, GL_FRAGMENT_SHADER);
                    auto log = gpc::gl::compileShader(fragment_shader, fragment_code);
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

            void _CanvasBase::define_viewport(int x, int y, int width, int height)
            {
                vp_width = width, vp_height = height;
            }

            void _CanvasBase::prepare_context()
            {
                EXEC_GL(glViewport, 0, 0, vp_width, vp_height);
                EXEC_GL(glUseProgram, program);
                gpc::gl::setUniform("vp_width" , 0, vp_width );
                gpc::gl::setUniform("vp_height", 1, vp_height);
            }

            void _CanvasBase::leave_context()
            {
                EXEC_GL(glUseProgram, 0);
            }

            void _CanvasBase::draw_rect(int x, int y, int w, int h)
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
            }

            auto _CanvasBase::register_rgba_image(size_t width, size_t height, const RGBA32 *pixels) -> image_handle_t
            {
                auto i = textures.size();
                textures.resize(i + 1);
                EXEC_GL(glGenTextures, 1, &textures[i]);
                EXEC_GL(glBindTexture, GL_TEXTURE_2D, textures[i]);
                EXEC_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                EXEC_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                EXEC_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                EXEC_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                EXEC_GL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                EXEC_GL(glBindTexture, GL_TEXTURE_2D, 0);
                return textures[i];
            }

            void _CanvasBase::fill_rect(int x, int y, int w, int h, const native_color_t &color)
            {
                gpc::gl::setUniform("color", 2, color.components);
                gpc::gl::setUniform("render_mode", 5, 1);

                draw_rect(x, y, w, h);
            }

            // TODO: rename (since it repeats the image) ? or overload fill_rect() ?

            void _CanvasBase::draw_image(int x, int y, int w, int h, image_handle_t image)
            {
                static const GLfloat black[4] = { 0, 0, 0, 0 };

                gpc::gl::setUniform("color", 2, black);
                GLint origin[2] = { x, y };
                gpc::gl::setUniform("sampler", 3, 0);
                gpc::gl::setUniform("origin", 4, origin);
                //EXEC_GL(glActiveTexture, GL_TEXTURE0);
                EXEC_GL(glBindTexture, GL_TEXTURE_2D, image);
                gpc::gl::setUniform("render_mode", 5, 2);

                std::vector<RGBA32> pixels(17 * 13);
                EXEC_GL(glGetTexImage, GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);

                draw_rect(x, y, w, h);
            }

        } // ns gl
    } // ns gui
} // gpc::gui::gl
