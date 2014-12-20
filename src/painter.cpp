#include <gpc/gui/gl/painter.hpp>

namespace gpc { namespace gui { namespace gl {

    std::string Painter::vertex_code {
        #include "vertex.glsl.h"
    };

} } } // gpc::gui::gl