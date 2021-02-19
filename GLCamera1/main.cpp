// stb
#include <stb_image.h>
// Internal
#include "input.h"
#include "camera.h"
#include "shaders.hpp"


constexpr auto APP_TITLE = "OpenGL Vector Camera Demo";
constexpr glm::vec3 CAMERA_ACCELERATION(8.0F, 8.0F, 8.0F);
constexpr float     CAMERA_FOVX = 90.0F;
constexpr glm::vec3 CAMERA_POS(0.0F, 1.0F, 0.0F);
constexpr float     CAMERA_SPEED_ROTATION   = 0.2F;
constexpr float     CAMERA_SPEED_FLIGHT_YAW = 100.0F;
constexpr glm::vec3 CAMERA_VELOCITY(2.0F, 2.0F, 2.0F);
constexpr float     CAMERA_ZFAR  = 100.0F;
constexpr float     CAMERA_ZNEAR = 0.1F;

constexpr float FLOOR_WIDTH  = 16.0F;
constexpr float FLOOR_HEIGHT = 16.0F;
constexpr float FLOOR_TILE_S = 8.0F;
constexpr float FLOOR_TILE_T = 8.0F;

static SDL_Window *g_pWindow;
static int         g_windowWidth;
static int         g_windowHeight;
static bool        g_hasFocus = true;

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[]) {
#if defined _DEBUG
    _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    ZoneScoped; // NOLINT

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("ERROR: Can not initailize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    g_windowWidth  = displayMode.w / 2;
    g_windowHeight = displayMode.h / 2;

    g_pWindow = SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 g_windowHeight, g_windowHeight, SDL_WINDOW_OPENGL);
    if(g_pWindow == nullptr) {
        SDL_Log("ERROR: Can not create SDL Window\n");
        return EXIT_FAILURE;
    }

#ifdef OPENGL_DEBUG
    int contextFlags;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
    contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
    // OpenGL 3.3 Core Profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GLContext context = SDL_GL_CreateContext(g_pWindow);
    if(context == nullptr) {
        SDL_Log("Can not create SDL2 context\n");
        return EXIT_FAILURE;
    }
    SDL_GL_SetSwapInterval(1);

    glbinding::initialize([](const char* name) {
        using proc = void(*)();

        return reinterpret_cast<proc>(SDL_GL_GetProcAddress(name));
    }, false);

    bool bRunning = true;
    while(bRunning) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                bRunning = false;
            }
            if(event.type == SDL_WINDOWEVENT) {
                const auto windowEvent = event.window;
                switch (windowEvent.event) {
                case SDL_WINDOWEVENT_CLOSE:
                break;
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    g_hasFocus = true;
                break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    g_hasFocus = false;
                break;
                }
            }
        }

        if (g_hasFocus) {
            glClear(GL_COLOR_BUFFER_BIT);
            SDL_GL_SwapWindow(g_pWindow);
            // UpdateFrame(GetElapsedTimeInSeconds());
            // RenderFrame();
            //SwapBuffers(g_hDC);
        } else {
            SDL_WaitEvent(&event);
        }
        FrameMark; // NOLINT
    }

    SDL_DestroyWindow(g_pWindow);
    SDL_Quit();

    return EXIT_SUCCESS;
}
