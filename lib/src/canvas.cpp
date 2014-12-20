#include <string>
#include <gpc/gui/gl/canvas.hpp>

namespace gpc { namespace gui { namespace gl {

    std::string Canvas::vertex_code {
        #include "vertex.glsl.h"
    };

} } } // gpc::gui::gl