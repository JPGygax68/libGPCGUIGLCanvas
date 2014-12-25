#include <cassert>
#include <cstdio>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_opengl.h>
#include <gpc/gl/wrappers.hpp>
#include <gpc/gui/gl/canvas.hpp>

using std::cout;
using gpc::gui::RGBAFloat;
using gpc::gui::RGBA32;
using gpc::gui::gl::Canvas;

static auto
makeColorInterpolatedRectangle(size_t width, size_t height, const std::array<RGBAFloat,4> &corner_colors) -> std::vector<RGBA32>
{
    std::vector<RGBA32> image(width * height);

    auto it = image.begin();
    for (auto y = 0U; y < height; y ++) {
        for (auto x = 0U; x < width; x ++) {
            RGBAFloat top    = interpolate(corner_colors[0], corner_colors[1], float(x) / float(width));
            RGBAFloat bottom = interpolate(corner_colors[2], corner_colors[3], float(x) / float(width));
            RGBAFloat color  = interpolate(top, bottom, float(y) / float(height));
            *it = fromFloat(color);
            //*it = { {255, 0, 0, 255} };
            it++;
        }
    }

    return image;
}

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

        SDL_GetWindowSize(window, &width, &height);

        auto context = SDL_GL_CreateContext(window);

        glewInit();

        EXEC_GL(glClearColor, 0.0f, 0.0f, 0.0f, 1.0f);

        typedef Canvas<true> MyCanvas;
        MyCanvas canvas;

        canvas.init();

        auto adaptToWindowDimensions = [&](unsigned int width, unsigned int height) {
            canvas.define_viewport(0, 0, width, height);
        };

        adaptToWindowDimensions(width, height);

        auto test_image = makeColorInterpolatedRectangle(17, 13, {{ {1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}, {1, 1, 1, 1}} });
        auto test_image_handle = canvas.register_rgba_image(17, 13, &test_image[0]);


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

            canvas.prepare_context();

            int y = 50;
            canvas.fill_rect(50, y, 150, 150, canvas.rgb_to_native({ 1, 0, 0 }));
            canvas.fill_rect(50 + 150 + 10, y, 150, 150, canvas.rgb_to_native({ 0, 1, 0 }));
            y += 150 + 10;
            canvas.fill_rect(50, y, 150, 150, canvas.rgb_to_native({ 0, 0, 1 }));
            canvas.fill_rect(50 + 150 + 10, y, 150, 150, canvas.rgb_to_native({ 1, 1, 1 }));
            
            y = 50;
            canvas.draw_image(400, y, 17, 13, test_image_handle);

            canvas.leave_context();

            SDL_GL_SwapWindow(window);

            SDL_Delay(20);
        }

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