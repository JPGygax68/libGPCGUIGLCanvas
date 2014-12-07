#include <iostream>
#include <thread>
#include <mutex>
#include <string>

#include <gpc/gui/gl/painter.hpp>

// TODO: make the following into a small, header-only library

#if !defined(GLDEBUG) && defined(_DEBUG)
#define GLDEBUG
#endif

#if defined (GLDEBUG)  

template<typename F, typename... Args>
auto glFunc(const char *text, int line, const char *file, F func, Args&&... args) -> decltype(func(std::forward<Args>(args)...))
{
    auto result = func(std::forward<Args>(args)...);
    //if (result == static_cast<decltype(result)>(0)) throw std::runtime_error(text);
    auto err = glGetError();
    if (err != 0) throw std::runtime_error(std::string(text) + " failed at line " + std::to_string(line) + " in file " + file);
    return result;
}

template<typename F, typename... Args>
auto glProc(F func, Args&&... args) -> decltype(func(std::forward<Args>(args)...))
{
    func(std::forward<Args>(args)...);
    auto err = glGetError();
    if (err != 0) throw std::runtime_error(std::string(text) + " failed at line " + std::to_string(line) + " in file " + file);
}

#define CALL_GL(func, ...) glFunc(#func, __LINE__, __FILE__, func, __VA_ARGS__)
#define EXEC_GL(func, ...) glProc(#func, __LINE__, __FILE__, func, __VA_ARGS__)

#else

#define CALL_GL(func) func
#define CALL_GL(func) func

#endif  

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