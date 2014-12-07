#include <iostream>
#include <thread>
#include <mutex>
#include <string>

#include <GL/glew.h>

#include <gpc/gl/wrappers.hpp>

#include <gpc/gui/gl/painter.hpp>

static char vertex_shader[] = {
#include <vertex.glsl.h>
};

namespace gpc { namespace gui { namespace gl {

    Painter::Painter():
        vertex_shader(0), fragment_shader(0)
    {
    }

    void Painter::init()
    {
        static std::once_flag flag;
        std::call_once(flag, []() { glewInit(); } );

        // Upload and compile our shader program
        vertex_shader = CALL_GL(glCreateShader, GL_FRAGMENT_SHADER);

        // TODO...
    }

} } } // gpc::gui::gl