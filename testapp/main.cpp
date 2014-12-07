#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <iostream>
using std::cout;

#include <gpc/gui/gl/painter.hpp>

int main(int argc, char *argv[])
{
    int width = 640, height = 480;

    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_Window *window = SDL_CreateWindow("Graphics Application", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        width, height, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    cout << "OpenGL Version " << glGetString(GL_VERSION) << "\n";

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

        static float delta = 0;
        delta += 0.002f;
        
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_TRIANGLES);
        glVertex3f(delta, 0, -5);
        glVertex3f(1 + delta, 0, -5);
        glVertex3f(delta, 1, -5);
        glEnd();
        
        SDL_GL_SwapWindow(window);
        SDL_Delay(20);
    }
}