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
using gpc::gui::RGBAFloat;
using gpc::gui::RGBA32;
using gpc::gui::gl::Canvas;

int main(int argc, char *argv[])
{
    typedef gpc::gui::gl::Canvas<true> canvas_t;
    typedef std::pair<SDL_Window*, SDL_GLContext> display_t;
    typedef gpc::gui::CanvasTestsuite<canvas_t, display_t> MyCanvasTestsuite;
    typedef MyCanvasTestsuite::context_t context_t;

    typedef MyCanvasTestsuite::draw_fn_t draw_fn_t;

    try {

        //int width = 640, height = 480;

        SDL_Init(SDL_INIT_VIDEO);
        IMG_Init(IMG_INIT_PNG);
    
        SDL_Rect main_display_bounds;
        SDL_GetDisplayBounds(0, &main_display_bounds);

        //auto adaptToWindowDimensions = [&](unsigned int width, unsigned int height) {
        //    canvas.define_viewport(0, 0, width, height);
        //};

        draw_fn_t draw_fn;

        auto draw_window = [&](display_t display, canvas_t &canvas) -> void {

            canvas.prepare_context();
            draw_fn(display, canvas);
            SDL_GL_SwapWindow(display.first);
            canvas.leave_context();
        };

        auto create_window = [&](int w, int h, draw_fn_t draw_fn_) -> context_t {

            Uint32 flags = SDL_WINDOW_OPENGL;
            if (w == 0 && h == 0) { 
                w = main_display_bounds.w, h = main_display_bounds.h; 
                flags |= SDL_WINDOW_BORDERLESS;
            }
            SDL_Window *window = SDL_CreateWindow("GPC GUI OpenGL Canvas test window", 0, 0, w, h, flags);
            SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
            glewInit();
            cout << "OpenGL Version " << glGetString(GL_VERSION) << "\n";
            EXEC_GL(glClearColor, 0.0f, 0.0f, 0.0f, 1.0f);

            int wr, hr;
            SDL_GetWindowSize(window, &wr, &hr);
            assert(wr == w && hr == h); // TODO: return the actual dimension to have test suite check?

            draw_fn = draw_fn_;
            
            canvas_t *canvas = new canvas_t();
            display_t disp = std::make_pair(window, gl_ctx);

            draw_window(disp, *canvas);

            return std::make_pair(disp, canvas);
        };

        auto destroy_window = [&](display_t disp) -> void {

            SDL_GL_DeleteContext(disp.second);
            SDL_DestroyWindow(disp.first);
        };

        MyCanvasTestsuite testsuite(create_window, destroy_window);

        testsuite.run_all_tests();

        /*
        SDL_Event event;
        while (1)
        {
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        testsuite.update_canvas_size(event.window.data1, event.window.data2);
                        // TODO: adapt to window dimensions adaptToWindowDimensions(event.window.data1, event.window.data2);
                    }
                    break;

                case SDL_QUIT: 
                    SDL_Quit(); 
                    return 0;
                }
            }

            SDL_Delay(20);
        }
        */

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