#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_opengl.h>
#include <gpc/gl/wrappers.hpp>
#include <gpc/gui/gl/canvas.hpp>
#include <gpc/fonts/RasterizedFont.hpp>
#include <gpc/fonts/cereal.hpp>
#include <cereal/archives/binary.hpp>

using std::cout;
using gpc::gui::RGBAFloat;
using gpc::gui::RGBA32;
using gpc::gui::gl::Canvas;

static char liberations_sans_data[] = {
#include "LiberationSans-Regular-20.rft.h"
};

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

        std::string data_string(liberations_sans_data, liberations_sans_data + sizeof(liberations_sans_data));
        std::stringstream sstr(data_string);
        cereal::BinaryInputArchive ar(sstr);
        gpc::fonts::RasterizedFont rfont;
        ar >> rfont;
        
        SDL_Init(SDL_INIT_VIDEO);
        IMG_Init(IMG_INIT_PNG);
    
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

        auto test_image = makeColorInterpolatedRectangle(170, 130, {{ {1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}, {1, 1, 1, 1}} });
        auto test_image_handle = canvas.register_rgba_image(170, 130, &test_image[0]);
        auto my_font = canvas.register_font(rfont);

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

            static const int SEPARATION = 20;
            int x = 50, y = 50, w, h;

            canvas.prepare_context();

            // TODO: use GPC layout module ?

            canvas.fill_rect(50, y, 150, 150, canvas.rgb_to_native({ 1, 0, 0 }));
            canvas.fill_rect(50 + 150 + 10, y, 150, 150, canvas.rgb_to_native({ 0, 1, 0 }));
            y += 150 + 10;
            canvas.fill_rect(50, y, 150, 150, canvas.rgb_to_native({ 0, 0, 1 }));
            canvas.fill_rect(50 + 150 + 10, y, 150, 150, canvas.rgb_to_native({ 1, 1, 1 }));
            
            x += 150 + 10 + 150 + SEPARATION, y = 50;
            // Single image
            canvas.draw_image(x, y, 170, 130, test_image_handle);
            x += 170 + SEPARATION;
            // Repeated image
            w = 2 * 170 + 8, h = 2 * 130 + 5;
            canvas.draw_image(x, y, w, h, test_image_handle);
            // Repeated, with clipping
            x += w + SEPARATION;
            canvas.set_clipping_rect(x + 20, y + 20, w - 40, h - 40);
            canvas.draw_image(x, y, w, h, test_image_handle);
            canvas.cancel_clipping();
            // Image with offset
            x += w + 20;
            canvas.draw_image(x, y, w, h, test_image_handle, 20, 20);
            y += 310;

            // Some text
            x = 50; y += 20;
            // Ascent (estimated) = 15; TODO: correct for top-down, but for bottom-up, descent should be used
            canvas.draw_text(my_font, x, y+15, "ABCDEFabcdef,;", 14);
            canvas.fill_rect(x, y+15 - 1, x + 150, 1, canvas.rgba_to_native({ 1, 0, 0, 0.5f }));
            y += 20;
            // With clipping
            y += 10;
            canvas.set_clipping_rect(x + 5, y+3, 100, 20 - 3 - 3);
            canvas.draw_text(my_font, x, y + 15, "Clipping clipping clipping", 26);
            canvas.cancel_clipping();

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