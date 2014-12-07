#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/glew.h>

namespace gpc {
    namespace gui {
        namespace gl {

            class Painter {
            public:
                Painter();

                void init();

            private:
                GLuint vertex_shader, fragment_shader;
            };

        } // ns gl
    } // ns gui
} // ns gpc