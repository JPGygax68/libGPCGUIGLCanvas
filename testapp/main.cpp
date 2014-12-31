#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_opengl.h>
#include <gpc/gl/wrappers.hpp>
#include <gpc/gui/gl/canvas.hpp>

#include <gpc/gui/canvas_testsuite.hpp>

using std::cout;

struct Harness {

    typedef gpc::gui::gl::Canvas<true>                              canvas_t;
    typedef std::pair<SDL_Window*, SDL_GLContext>                   display_t;
    typedef gpc::gui::CanvasTestSuite<canvas_t, display_t, Harness> TestSuite;
    typedef TestSuite::init_fn_t                                    init_fn_t;
    typedef TestSuite::cleanup_fn_t                                 cleanup_fn_t;
    typedef TestSuite::draw_fn_t                                    draw_fn_t;
    typedef TestSuite::context_t                                    context_t;

    Harness(): window(nullptr), gl_ctx(0), canvas(nullptr) {}

    void init() 
    {
        SDL_GetDisplayBounds(0, &main_display_bounds);
    }

    auto create_window(int w, int h, init_fn_t init_fn, draw_fn_t draw_fn) 
        -> context_t 
    {
        if (window) throw std::runtime_error("This test harness can only open one window/display at a time.");
        assert(gl_ctx == 0);

        Uint32 flags = SDL_WINDOW_OPENGL;
        if (w == 0 && h == 0) {
            w = main_display_bounds.w, h = main_display_bounds.h;
            flags |= SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS;
        }
        window = SDL_CreateWindow("GPC GUI OpenGL Canvas test window", 0, 0, w, h, flags);
        gl_ctx = SDL_GL_CreateContext(window);
        glewInit();
        cout << "OpenGL Version " << glGetString(GL_VERSION) << "\n";
        EXEC_GL(glClearColor, 0.0f, 0.0f, 0.0f, 1.0f);

        int wr, hr;
        SDL_GetWindowSize(window, &wr, &hr);
        assert(wr == w && hr == h); // TODO: return the actual dimension to have test suite check?

        canvas_t *canvas = new canvas_t();
        display_t disp = std::make_pair(window, gl_ctx);

        if (init_fn) init_fn(disp, canvas);

        draw_fn(disp, canvas);

        return std::make_pair(disp, canvas);
    }

    void destroy_window(display_t disp, cleanup_fn_t cleanup_fn) 
    {
        assert(canvas);

        if (cleanup_fn) cleanup_fn(disp, canvas);

        delete canvas;
        canvas = nullptr;

        assert(disp.second == gl_ctx);
        SDL_GL_DeleteContext(disp.second);
        gl_ctx = 0;

        assert(disp.first == window);
        SDL_DestroyWindow(disp.first);
        window = 0;
    }

    void present_window(display_t display, draw_fn_t draw_fn)
    {
        assert(display.first == window);
        SDL_ShowWindow(display.first);

        bool running = true;
        while (running)
        {
            SDL_Event event;
            while (SDL_WaitEvent(&event))
            {
                switch (event.type)
                {
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        //test_suite.update_canvas_size(event.window.data1, event.window.data2);
                        // TODO: adapt to window dimensions adaptToWindowDimensions(event.window.data1, event.window.data2);
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                        render_content(display, canvas, draw_fn);
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        running = false;
                    }
                    break;

                case SDL_QUIT:
                    SDL_Quit();
                    running = false;
                    return;
                }
            }

            //SDL_Delay(20);
        }
    }

    void render_content(display_t display, canvas_t *canvas, draw_fn_t draw_fn)
    {
        canvas->prepare_context();
        draw_fn(display, canvas);
        SDL_GL_SwapWindow(display.first);
        canvas->leave_context();
    };

private:

    SDL_Rect            main_display_bounds;
    SDL_Window          *window;
    SDL_GLContext       gl_ctx;
    canvas_t            *canvas;
};

int main(int argc, char *argv[])
{
    try {

        //int width = 640, height = 480;

        SDL_Init(SDL_INIT_VIDEO);
        IMG_Init(IMG_INIT_PNG);

        Harness::TestSuite test_suite;

        //auto adaptToWindowDimensions = [&](unsigned int width, unsigned int height) {
        //    canvas.define_viewport(0, 0, width, height);
        //};

        test_suite.run_all_tests();

        return 0;
    }
    catch(const std::exception &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Press RETURN to terminate" << std::endl;
        std::cin.ignore(1);
    }
    catch(...) {}

    return 1;
}