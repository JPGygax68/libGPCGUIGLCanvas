#include <cassert>
#include <cstdio>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_opengl.h>
#include <iostream>
using std::cout;

#include <gpc/gui/gl/painter.hpp>

int main(int argc, char *argv[])
{
    try {

        int width = 640, height = 480;

        SDL_Init(SDL_INIT_VIDEO);
        IMG_Init(IMG_INIT_PNG);
    
        auto img_surf = IMG_Load("../../assets/uniform_fill.png");

        SDL_Rect disp_bounds;
        SDL_GetDisplayBounds(0, &disp_bounds);
        SDL_Window *window = SDL_CreateWindow("Graphics Application",
            0, 0, disp_bounds.w, disp_bounds.h,
            SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);
        SDL_GLContext glcontext = SDL_GL_CreateContext(window);
        cout << "OpenGL Version " << glGetString(GL_VERSION) << "\n";

        auto context = SDL_GL_CreateContext(window);

        glewInit();

        auto adaptToWindowDimensions = [](unsigned int width, unsigned int height) {
            glViewport(0, 0, width, height);
            glLoadIdentity();
            glFrustum(-1, 1, -(float)height / width, (float)height / width, 1, 500);
        };
    
        glClearColor(0, 0, 0, 1);

        adaptToWindowDimensions(width, height);

        gpc::gui::gl::Painter painter;

        painter.init();

        SDL_Event event;
        while (1)
        {
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        adaptToWindowDimensions(event.window.data1, event.window.data2);
                    }
                    break;

                case SDL_QUIT: 
                    SDL_GL_DeleteContext(glcontext); 
                    SDL_DestroyWindow(window); 
                    SDL_Quit(); 
                    return 0;
                }
            }

            painter.fill_rect(10, 10, 300, 150, painter.rgb_to_native({1, 1, 1}) );

            SDL_GL_SwapWindow(window);

            SDL_Delay(20);
        }

        return 0;
    }
    catch(const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    catch(...) {}

    return 1;
}