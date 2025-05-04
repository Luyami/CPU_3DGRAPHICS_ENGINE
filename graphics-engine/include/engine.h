#ifndef ENGINE_H
#define ENGINE_H

#include "../../sdlbuild/include/SDL2/SDL.h"
#include "../../glew/glew/include/GL/glew.h"

#include "../../imgui/include/imgui.h"
#include "../../imgui/include/imgui_impl_sdl2.h"
#include "../../imgui/include/imgui_impl_sdlrenderer2.h"
#include "../../imgui/include/imgui_impl_opengl3.h"

#include "../include/graphics-engine.h"

#include <queue>
#include <thread>
#include <mutex>

class Engine{
    private:
        //config params
        static double velFRONT;
        static double velRIGHT;
        static double velUP;
        static double speed;

        static int RT_RAYS;

        static bool IS_WIREFRAME_ENABLED;
        static bool IS_OPTIMIZATION_ENABLED;
        static bool PREDICT_PIXEL;
        static bool IS_BILINEAR_ENABLED;
        static bool USING_ORTHOGRAPHIC_PROJECTION;
        static bool USING_OBLIQUE_PROJECTION;

        //Parâmetros de desenho
        static bool DRAWING;
        static float PEN_COLOR[3];
        static float PEN_THICKNESS;

        static bool MOUSE_LEFTB_HELD;
        static char name_buffer[128];
        static float pos_buffer[3];
        static float camera_fov;
        static Object* selectedObject;
        static Light* selectedLight;
        //**

        static float LIGHT_COLOR_RGB[3];

        static bool onLoop;

        static double DELTA_TIME;

        static SDL_Window* config_window;
        static SDL_Window* scene_window;

        static Scene* scene;

        static void __init_ConfigWindow(Scene& s, int w, int h);
        static void __init_SceneWindow(Scene& s, int w, int h);

        static void __render_step();
        static void __interface_step();
        static void __input_step();
        static void __event_step(SDL_Event& e);

        static void __engine_loop();

    public:
        static void Start(Scene& s, int scene_width, int scene_height, int config_width, int config_height);

        static SDL_Window* getConfigWindow();
        static SDL_Window* getSceneWindow();
};

#endif