//-----------------------------------------------------------------------------
// Copyright (c) 2006-2008 dhpoware. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------
//
// This is the first demo in the OpenGL camera demo series. In this demo we
// implement the classic vector based camera. The camera supports 2 modes of
// operation: first person camera mode, and flight simulator camera mode. We
// don't implement the third person camera in this demo. A future demo will
// explore the implementation of the third person camera.
//
//-----------------------------------------------------------------------------
// stb
#include <stb_image.h>
// Internal
#include "input.h"
#include "camera.h"
#include "shaders.hpp"


//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------
// clang-format off
using wglCreateContextAttribsARBFunc = HGLRC (*)(HDC, HGLRC, const int*);
using wglChoosePixelFormatARBFunc    = BOOL (*)(HDC , const int*, const FLOAT*, UINT, int*, UINT*);
// clang-format on

constexpr auto APP_TITLE = "OpenGL Vector Camera Demo";

// Windows Vista compositing support.
#if !defined(PFD_SUPPORT_COMPOSITION)
#define PFD_SUPPORT_COMPOSITION 0x00008000
#endif

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

//-----------------------------------------------------------------------------
// Globals.
//-----------------------------------------------------------------------------

static HWND      g_hWnd;
static HDC       g_hDC;
static HGLRC     g_hContext;
static HINSTANCE g_hInstance;
static int       g_framesPerSecond;
static int       g_windowWidth;
static int       g_windowHeight;
static int       g_msaaSamples;
static int       g_maxAnisotrophy;
static bool      g_isFullScreen;
static bool      g_hasFocus;
static bool      g_enableVerticalSync;
static bool      g_displayHelp;
static bool      g_flightModeEnabled;
static GLuint    g_floorColorMapTexture;
static GLuint    g_floorLightMapTexture;
static Camera    g_camera;
static glm::vec3 g_cameraBoundsMax;
static glm::vec3 g_cameraBoundsMin;
static float     g_cameraRotationSpeed = CAMERA_SPEED_ROTATION;

static GLuint g_VAO = 0;
static GLuint g_VBO = 0;
static GLuint g_EBO = 0;
static GLuint g_UBO = 0;
static GLuint g_Program = 0;

static GLint g_uTexture0Locaion;
static GLint g_uTexture1Locaion;

wglCreateContextAttribsARBFunc wglCreateContextAttribsARB;
wglChoosePixelFormatARBFunc    wglChoosePixelFormatARB;

//-----------------------------------------------------------------------------
// Functions Prototypes.
//-----------------------------------------------------------------------------

void    Cleanup();
void    CleanupApp();
HWND    CreateAppWindow(const WNDCLASSEX &wcl, const char *pszTitle);
void    EnableVerticalSync(bool enableVerticalSync);
bool    ExtensionSupported(const char *pszExtensionName);
float   GetElapsedTimeInSeconds();
void    GetMovementDirection(glm::vec3 &direction);
bool    Init();
void    InitApp();
void    InitOpenglExtensions();
void    InitGL();
void    InitImgui();
GLuint  LoadTexture(const char *pszFilename);
GLuint  LoadTexture(const char *pszFilename, GLenum magFilter, GLenum minFilter,
                    GLenum wrapS, GLenum wrapT);
void    Log(const char *pszMessage);
void    PerformCameraCollisionDetection();
void    ProcessUserInput();
void    RenderFloor();
void    RenderFrame();
void    RenderText();
void    SetProcessorAffinity();
void    ToggleFullScreen();
void    UpdateCamera(float elapsedTimeSec);
void    UpdateFrame(float elapsedTimeSec);
void    UpdateFrameRate(float elapsedTimeSec);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void    createBuffers();
void    createProgram();

//-----------------------------------------------------------------------------
// Functions.
//-----------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, int nShowCmd) {
#if defined _DEBUG
    _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    ZoneScoped; // NOLINT

    MSG msg = {0};
    WNDCLASSEX wcl = {0};

    // clang-format off
    wcl.cbSize        = sizeof(wcl);
    wcl.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcl.lpfnWndProc   = WindowProc;
    wcl.cbClsExtra    = 0;
    wcl.cbWndExtra    = 0;
    wcl.hInstance     = g_hInstance = hInstance;
    wcl.hIcon         = LoadIcon(0, IDI_APPLICATION);
    wcl.hCursor       = LoadCursor(0, IDC_ARROW);
    wcl.hbrBackground = 0;
    wcl.lpszMenuName  = 0;
    wcl.lpszClassName = "GLWindowClass";
    wcl.hIconSm       = 0;
    // clang-format on

    if (!RegisterClassEx(&wcl)) {
        return EXIT_FAILURE;
    }

    const auto hWnd = CreateAppWindow(wcl, APP_TITLE);
    if(!hWnd) {
        Log("Error: CreateAppWindow()");
    }
    g_hWnd = hWnd;

    if(g_hWnd) {
        SetProcessorAffinity();

        if(Init()) {
            ShowWindow(g_hWnd, nShowCmd);
            UpdateWindow(g_hWnd);

            while(true) {
                while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
                    if (msg.message == WM_QUIT) {
                        break;
                    }

                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                if (msg.message == WM_QUIT) {
                    break;
                }

                if (g_hasFocus) {
                    UpdateFrame(GetElapsedTimeInSeconds());
                    RenderFrame();
                    SwapBuffers(g_hDC);
                } else {
                    WaitMessage();
                }
                FrameMark; // NOLINT
            }
        }

        Cleanup();
        UnregisterClass(wcl.lpszClassName, hInstance);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ZoneScoped; // NOLINT
    switch (msg) {
    case WM_ACTIVATE:
        switch (wParam) {
        default:
            break;

        case WA_ACTIVE:
        case WA_CLICKACTIVE:
            Mouse::instance().attach(hWnd);
            g_hasFocus = true;
            break;

        case WA_INACTIVE:
            if (g_isFullScreen) {
                ShowWindow(hWnd, SW_MINIMIZE);
            }
            Mouse::instance().detach();
            g_hasFocus = false;
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        g_windowWidth  = static_cast<int>(LOWORD(lParam));
        g_windowHeight = static_cast<int>(HIWORD(lParam));
        break;

    default:
        Mouse::instance().handleMsg(hWnd, msg, wParam, lParam);
        Keyboard::instance().handleMsg(hWnd, msg, wParam, lParam);
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Cleanup() {
    ZoneScoped; // NOLINT
    CleanupApp();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_hDC) {
        if(g_hContext) {
            wglMakeCurrent(g_hDC, 0);
            wglDeleteContext(g_hContext);
        }

        ReleaseDC(g_hWnd, g_hDC);
    }
}

void CleanupApp() {
    ZoneScoped; // NOLINT
    //g_font.destroy();

    glBindVertexArray(0);
    if(g_VAO != 0) {
      glDeleteVertexArrays(1, &g_VAO);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if(g_VBO != 0) {
      glDeleteBuffers(1, &g_VBO);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    if(g_EBO) {
      glDeleteBuffers(1, &g_EBO);
    }

    glUseProgram(0);
    if(g_Program) {
      glDeleteProgram(g_Program);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    if(g_UBO != 0) {
        glDeleteBuffers(1, &g_UBO);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    if (g_floorColorMapTexture) {
        glDeleteTextures(1, &g_floorColorMapTexture);
    }

    if (g_floorLightMapTexture) {
        glDeleteTextures(1, &g_floorLightMapTexture);
    }
}

HWND CreateAppWindow(const WNDCLASSEX &wcl, const char *pszTitle) {
    ZoneScoped; // NOLINT
    // Create a window that is centered on the desktop. It's exactly 1/4 the
    // size of the desktop. Don't allow it to be resized.

    DWORD wndExStyle = WS_EX_OVERLAPPEDWINDOW;
    DWORD wndStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                     WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    // clang-format off
    HWND hWnd = CreateWindowEx(
      wndExStyle,
      wcl.lpszClassName,
      pszTitle,
      wndStyle,
      0,
      0,
      0,
      0,
      0,
      0,
      wcl.hInstance,
      0
    );
    // clang-format on

    if (!hWnd) {
        Log("Error: CreateWindowEx()");
    }

    const int screenWidth      = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight     = GetSystemMetrics(SM_CYSCREEN);
    const int halfScreenWidth  = screenWidth / 2;
    const int halfScreenHeight = screenHeight / 2;
    const int left = (screenWidth - halfScreenWidth) / 2;
    const int top  = (screenHeight - halfScreenHeight) / 2;

    RECT rc = {0};
    SetRect(&rc, left, top, left + halfScreenWidth, top + halfScreenHeight);

    AdjustWindowRectEx(&rc, wndStyle, FALSE, wndExStyle);
    MoveWindow(hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

    GetClientRect(hWnd, &rc);
    g_windowWidth  = rc.right - rc.left;
    g_windowHeight = rc.bottom - rc.top;

    return hWnd;
}

void EnableVerticalSync(bool enableVerticalSync) {
    ZoneScoped; // NOLINT
    // WGL_EXT_swap_control.

    typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(GLint);

    static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
        reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(
        wglGetProcAddress("wglSwapIntervalEXT"));

    if (wglSwapIntervalEXT)
    {
        wglSwapIntervalEXT(enableVerticalSync ? 1 : 0);
        g_enableVerticalSync = enableVerticalSync;
    }
}

bool ExtensionSupported(const char *pszExtensionName) {
    ZoneScoped; // NOLINT
    static const char *pszGLExtensions = 0;
    static const char *pszWGLExtensions = 0;

    if (!pszGLExtensions)
        pszGLExtensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

    if (!pszWGLExtensions) {
        // WGL_ARB_extensions_string.

        typedef const char *(WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC);

        PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
            reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGARBPROC>(
            wglGetProcAddress("wglGetExtensionsStringARB"));

        if (wglGetExtensionsStringARB)
            pszWGLExtensions = wglGetExtensionsStringARB(wglGetCurrentDC());
    }

    if (!strstr(pszGLExtensions, pszExtensionName))
    {
        if (!strstr(pszWGLExtensions, pszExtensionName))
            return false;
    }

    return true;
}

float GetElapsedTimeInSeconds() {
    ZoneScoped; // NOLINT
    // Returns the elapsed time (in seconds) since the last time this function
    // was called. This elaborate setup is to guard against large spikes in
    // the time returned by QueryPerformanceCounter().

    static const int MAX_SAMPLE_COUNT = 50;

    static float frameTimes[MAX_SAMPLE_COUNT];
    static float timeScale = 0.0f;
    static float actualElapsedTimeSec = 0.0f;
    static INT64 freq = 0;
    static INT64 lastTime = 0;
    static int sampleCount = 0;
    static bool initialized = false;

    INT64 time = 0;
    float elapsedTimeSec = 0.0f;

    if (!initialized) {
        initialized = true;
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&lastTime));
        timeScale = 1.0F / static_cast<float>(freq);
    }

    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&time));
    elapsedTimeSec = static_cast<float>(time - lastTime) * timeScale;
    lastTime = time;

    if (fabsf(elapsedTimeSec - actualElapsedTimeSec) < 1.0F) {
        memmove(&frameTimes[1], frameTimes, sizeof(frameTimes) - sizeof(frameTimes[0]));
        frameTimes[0] = elapsedTimeSec;

        if (sampleCount < MAX_SAMPLE_COUNT) {
            ++sampleCount;
        }
    }

    actualElapsedTimeSec = 0.0f;

    for (int i = 0; i < sampleCount; ++i) {
        actualElapsedTimeSec += frameTimes[i];
    }

    if (sampleCount > 0) {
        actualElapsedTimeSec /= static_cast<float>(sampleCount);
    }

    return actualElapsedTimeSec;
}

void GetMovementDirection(glm::vec3 &direction) {
    ZoneScoped; // NOLINT
    static bool moveForwardsPressed  = false;
    static bool moveBackwardsPressed = false;
    static bool moveRightPressed     = false;
    static bool moveLeftPressed      = false;
    static bool moveUpPressed        = false;
    static bool moveDownPressed      = false;

    glm::vec3 velocity   = g_camera.getCurrentVelocity();
    Keyboard &keyboard = Keyboard::instance();

    direction = {0.0f, 0.0f, 0.0f};

    if (keyboard.keyDown(Keyboard::KEY_W)) {
        if (!moveForwardsPressed) {
            moveForwardsPressed = true;
            g_camera.setCurrentVelocity(velocity.x, velocity.y, 0.0f);
        }

        direction.z += 1.0f;
    } else {
        moveForwardsPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_S)) {
        if (!moveBackwardsPressed) {
            moveBackwardsPressed = true;
            g_camera.setCurrentVelocity(velocity.x, velocity.y, 0.0f);
        }

        direction.z -= 1.0f;
    } else {
        moveBackwardsPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_D)) {
        if (!moveRightPressed) {
            moveRightPressed = true;
            g_camera.setCurrentVelocity(0.0f, velocity.y, velocity.z);
        }

        direction.x += 1.0f;
    } else {
        moveRightPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_A)) {
        if (!moveLeftPressed) {
            moveLeftPressed = true;
            g_camera.setCurrentVelocity(0.0f, velocity.y, velocity.z);
        }

        direction.x -= 1.0f;
    } else {
        moveLeftPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_E)) {
        if (!moveUpPressed) {
            moveUpPressed = true;
            g_camera.setCurrentVelocity(velocity.x, 0.0f, velocity.z);
        }

        direction.y += 1.0f;
    } else {
        moveUpPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_Q)) {
        if (!moveDownPressed) {
            moveDownPressed = true;
            g_camera.setCurrentVelocity(velocity.x, 0.0f, velocity.z);
        }

        direction.y -= 1.0f;
    } else {
        moveDownPressed = false;
    }
}

bool Init() {
    ZoneScoped; // NOLINT

    try {
        InitGL();
        InitApp();
    } catch (const std::exception &e) {
        const auto errorMessage = fmt::format("Application initialization failed!\n\n{}", e.what());
        Log(errorMessage.c_str());
        return false;
    }

    return true;
}

void InitGL() {
  ZoneScoped;  // NOLINT

  g_hDC = GetDC(g_hWnd);
  if(!g_hDC) {
    throw std::runtime_error("GetDC() failed.");
  }

  InitOpenglExtensions();

  // clang-format off
  constexpr auto WGL_DRAW_TO_WINDOW_ARB = 0x2001;
  constexpr auto WGL_ACCELERATION_ARB   = 0x2003;
  constexpr auto WGL_SUPPORT_OPENGL_ARB = 0x2010;
  constexpr auto WGL_DOUBLE_BUFFER_ARB  = 0x2011;
  constexpr auto WGL_PIXEL_TYPE_ARB     = 0x2013;
  constexpr auto WGL_COLOR_BITS_ARB     = 0x2014;
  constexpr auto WGL_DEPTH_BITS_ARB     = 0x2022;
  constexpr auto WGL_STENCIL_BITS_ARB   = 0x2023;

  constexpr auto WGL_FULL_ACCELERATION_ARB = 0x2027;
  constexpr auto WGL_TYPE_RGBA_ARB         = 0x202B;
  // Now we can choose a pixel format the modern way, using wglChoosePixelFormatARB.
  constexpr int pixel_format_attribs[] = {
      WGL_DRAW_TO_WINDOW_ARB,     1,
      WGL_SUPPORT_OPENGL_ARB,     1,
      WGL_DOUBLE_BUFFER_ARB,      1,
      WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
      WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
      WGL_COLOR_BITS_ARB,         32,
      WGL_DEPTH_BITS_ARB,         24,
      WGL_STENCIL_BITS_ARB,       8,
      0
  };
  // clang-format on

  int pixel_format;
  UINT num_formats;
  wglChoosePixelFormatARB(g_hDC, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
  if(num_formats == 0) {
    throw std::runtime_error("wglChoosePixelFormatARB() failed.");
  }

  PIXELFORMATDESCRIPTOR pfd;
  DescribePixelFormat(g_hDC, pixel_format, sizeof(pfd), &pfd);
  if(!SetPixelFormat(g_hDC, pixel_format, &pfd)) {
    throw std::runtime_error("SetPixelFormat() failed.");
  }

  constexpr auto WGL_CONTEXT_MAJOR_VERSION_ARB = 0x2091;
  constexpr auto WGL_CONTEXT_MINOR_VERSION_ARB = 0x2092;
  constexpr auto WGL_CONTEXT_PROFILE_MASK_ARB = 0x9126;
  constexpr auto WGL_CONTEXT_FLAGS_ARB = 0x2094;
  constexpr auto WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;

  constexpr auto WGL_CONTEXT_DEBUG_BIT_ARB = 0x0001;
  constexpr auto WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB = 0x0002;

  // clang-format off
  // Specify that we want to create an OpenGL 3.3 core profile context
  constexpr int attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 6,
      WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef OPENG_DEBUG
      WGL_CONTEXT_FLAGS_ARB       ,  WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
      0,
  };
  // clang-format on

  g_hContext = wglCreateContextAttribsARB(g_hDC, 0, attribs);
  if(!g_hContext) {
    throw std::runtime_error("wglCreateContextAttribsARB() failed.");
  }

  if(!wglMakeCurrent(g_hDC, g_hContext)) {
    throw std::runtime_error("wglMakeCurrent() failed.");
  }

  // glbinding::GetProcAddress
  glbinding::initialize(glbinding::GetProcAddress());
  //f(gl3wInit() != GL3W_OK) {
  // throw std::runtime_error("gl3wInit() failed.");
  //

#ifdef OPENG_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(
    [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
      /*
      static FILE* pLogging = fopen("Logging.txt", "w");
      fprintf(pLogging,
              "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
              (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
              type,
              severity,
              message);
    */
    },
    0);
#endif

  EnableVerticalSync(false);

  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &g_maxAnisotrophy);

  InitImgui();
}

void InitImgui() {
    ZoneScoped;  // NOLINT

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    [[maybe_unused]] ImGuiIO &io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplOpenGL3_Init();
}

void InitApp() {
  ZoneScoped; // NOLINT

  // Load textures.
  g_floorColorMapTexture = LoadTexture("floor_color_map.jpg");
  if(!g_floorColorMapTexture) {
    throw std::runtime_error("Failed to load texture: floor_color_map.jpg");
  }

  g_floorLightMapTexture = LoadTexture("floor_light_map.jpg");
  if(!g_floorLightMapTexture) {
    throw std::runtime_error("Failed to load texture: floor_light_map.jpg");
  }

  // Setup camera.
  g_camera.perspective(
    CAMERA_FOVX,
    static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight),
    CAMERA_ZNEAR,
    CAMERA_ZFAR
  );

  g_camera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON);
  g_camera.setPosition(CAMERA_POS);
  g_camera.setAcceleration(CAMERA_ACCELERATION);
  g_camera.setVelocity(CAMERA_VELOCITY);

  g_cameraBoundsMax = {FLOOR_WIDTH / 2.0f, 4.0f, FLOOR_HEIGHT / 2.0f};
  g_cameraBoundsMin = {-FLOOR_WIDTH / 2.0f, CAMERA_POS.y, -FLOOR_HEIGHT / 2.0f};

  Mouse::instance().hideCursor(true);
  Mouse::instance().moveToWindowCenter();

  createBuffers();
  createProgram();
}

void InitOpenglExtensions() {
  ZoneScoped;  // NOLINT

  // clang-format off
  WNDCLASSA tempWindowClass = {};
  tempWindowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  tempWindowClass.lpfnWndProc   = DefWindowProcA;
  tempWindowClass.hInstance     = GetModuleHandle(0);
  tempWindowClass.lpszClassName = "Temp";
  // clang-format on

  if(!RegisterClassA(&tempWindowClass)) {
    throw std::runtime_error("RegisterClassA() failed.");
  }

  HWND dummyWindow = CreateWindowExA(
    0,
    tempWindowClass.lpszClassName,
    "Dummy OpenGL Window",
    0,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    0,
    0,
    tempWindowClass.hInstance,
    0
  );

  if(!dummyWindow) {
    throw std::runtime_error("CreateWindowExA() failed.");
  }

  HDC dummyDC = GetDC(dummyWindow);

  // clang-format off
  PIXELFORMATDESCRIPTOR pfd = {};
  pfd.nSize        = sizeof(pfd);
  pfd.nVersion     = 1;
  pfd.iPixelType   = PFD_TYPE_RGBA;
  pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.cColorBits   = 32;
  pfd.cAlphaBits   = 8;
  pfd.iLayerType   = PFD_MAIN_PLANE;
  pfd.cDepthBits   = 24;
  pfd.cStencilBits = 8;
  // clang-format on

  int pixelFormat = ChoosePixelFormat(dummyDC, &pfd);
  if(!pixelFormat) {
    throw std::runtime_error("ChoosePixelFormat() failed.");
  }
  if(!SetPixelFormat(dummyDC, pixelFormat, &pfd)) {
    throw std::runtime_error("SetPixelFormat() failed.");
  }

  HGLRC dummyContext = wglCreateContext(dummyDC);
  if(!dummyContext) {
    throw std::runtime_error("wglCreateContext() failed.");
  }

  if(!wglMakeCurrent(dummyDC, dummyContext)) {
    throw std::runtime_error("wglMakeCurrent() failed.");
  }

  wglCreateContextAttribsARB = (wglCreateContextAttribsARBFunc)wglGetProcAddress("wglCreateContextAttribsARB");
  wglChoosePixelFormatARB    = (wglChoosePixelFormatARBFunc)wglGetProcAddress("wglChoosePixelFormatARB");

  wglMakeCurrent(dummyDC, 0);
  wglDeleteContext(dummyContext);
  ReleaseDC(dummyWindow, dummyDC);
  DestroyWindow(dummyWindow);
}

GLuint LoadTexture(const char *pszFilename) {
    ZoneScoped; // NOLINT

    return LoadTexture(pszFilename, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR,
        GL_REPEAT, GL_REPEAT);
}

GLuint LoadTexture(const char *pszFilename, GLenum magFilter, GLenum minFilter,
                   GLenum wrapS, GLenum wrapT) {
    ZoneScoped; // NOLINT

    GLuint id = 0;
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    void *pImage = stbi_load(pszFilename, &width, &height, &channels, 4);

    if(pImage != nullptr) {
        glCreateTextures(GL_TEXTURE_2D, 1, &id);

        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, magFilter);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S,     wrapS);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T,     wrapT);

        glTextureStorage2D(id,  1, GL_RGBA8, width, height);
        glTextureSubImage2D(id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pImage);
        if(minFilter == GL_LINEAR_MIPMAP_LINEAR) {
          glGenerateTextureMipmap(id);
        }
        glTextureParameteri(id, GL_TEXTURE_MAX_ANISOTROPY, g_maxAnisotrophy);

        stbi_image_free(pImage);
    }

    return id;
}

void Log(const char *pszMessage) {
    ZoneScoped; // NOLINT

    MessageBox(0, pszMessage, "Error", MB_ICONSTOP);
}

void PerformCameraCollisionDetection() {
    ZoneScoped; // NOLINT
    const glm::vec3 &pos = g_camera.getPosition();
    glm::vec3 newPos(pos);

    if (pos.x > g_cameraBoundsMax.x)
        newPos.x = g_cameraBoundsMax.x;

    if (pos.x < g_cameraBoundsMin.x)
        newPos.x = g_cameraBoundsMin.x;

    if (pos.y > g_cameraBoundsMax.y)
        newPos.y = g_cameraBoundsMax.y;

    if (pos.y < g_cameraBoundsMin.y)
        newPos.y = g_cameraBoundsMin.y;

    if (pos.z > g_cameraBoundsMax.z)
        newPos.z = g_cameraBoundsMax.z;

    if (pos.z < g_cameraBoundsMin.z)
        newPos.z = g_cameraBoundsMin.z;

    g_camera.setPosition(newPos);
}

void ProcessUserInput() {
    ZoneScoped; // NOLINT

    Keyboard &keyboard = Keyboard::instance();
    Mouse &mouse = Mouse::instance();

    if (keyboard.keyPressed(Keyboard::KEY_ESCAPE))
        PostMessage(g_hWnd, WM_CLOSE, 0, 0);

    if (keyboard.keyDown(Keyboard::KEY_LALT) || keyboard.keyDown(Keyboard::KEY_RALT)) {
        if (keyboard.keyPressed(Keyboard::KEY_ENTER))
            ToggleFullScreen();
    }

    if (keyboard.keyPressed(Keyboard::KEY_H))
        g_displayHelp = !g_displayHelp;

    if (keyboard.keyPressed(Keyboard::KEY_M))
        mouse.smoothMouse(!mouse.isMouseSmoothing());

    if (keyboard.keyPressed(Keyboard::KEY_V))
        EnableVerticalSync(!g_enableVerticalSync);

    if (keyboard.keyPressed(Keyboard::KEY_ADD) ||
        keyboard.keyPressed(Keyboard::KEY_NUMPAD_ADD)) {
        g_cameraRotationSpeed += 0.01f;

        if (g_cameraRotationSpeed > 1.0f)
            g_cameraRotationSpeed = 1.0f;
    }

    if (keyboard.keyPressed(Keyboard::KEY_SUBTRACT) ||
        keyboard.keyPressed(Keyboard::KEY_NUMPAD_SUBTRACT)) {
        g_cameraRotationSpeed -= 0.01f;

        if (g_cameraRotationSpeed <= 0.0f)
            g_cameraRotationSpeed = 0.01f;
    }

    if (keyboard.keyPressed(Keyboard::KEY_PERIOD)) {
        mouse.setWeightModifier(mouse.weightModifier() + 0.1f);

        if (mouse.weightModifier() > 1.0f)
            mouse.setWeightModifier(1.0f);
    }

    if (keyboard.keyPressed(Keyboard::KEY_COMMA)) {
        mouse.setWeightModifier(mouse.weightModifier() - 0.1f);

        if (mouse.weightModifier() < 0.0f) {
            mouse.setWeightModifier(0.0f);
        }
    }

    if (keyboard.keyPressed(Keyboard::KEY_SPACE)) {
        g_flightModeEnabled = !g_flightModeEnabled;

        if (g_flightModeEnabled) {
            g_camera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_FLIGHT);
        } else {
            const glm::vec3 &cameraPos = g_camera.getPosition();

            g_camera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON);
            g_camera.setPosition(cameraPos.x, CAMERA_POS.y, cameraPos.z);
        }
    }
}

void RenderFloor() {
  ZoneScoped; // NOLINT

  glUseProgram(g_Program);

  constexpr auto FloorTextureId = 0;
  glBindTextureUnit(FloorTextureId, g_floorColorMapTexture);
  glUniform1i(g_uTexture0Locaion, FloorTextureId);

  constexpr auto FloorLightTextureId = 1;
  glBindTextureUnit(FloorLightTextureId, g_floorLightMapTexture);
  glUniform1i(g_uTexture1Locaion, FloorLightTextureId);

  glBindBuffer(GL_ARRAY_BUFFER, g_VBO);

  constexpr auto PositionID = 0;
  constexpr auto UV1ID      = 1;
  constexpr auto UV2ID      = 2;

  glEnableVertexAttribArray(PositionID);
  glEnableVertexAttribArray(UV1ID);
  glEnableVertexAttribArray(UV2ID);

  const auto offset1 = (GLvoid *)(4 * sizeof(glm::vec3));
  const auto offset2 = (GLvoid *)(4 * sizeof(glm::vec3) + 4 * sizeof(glm::vec2));

  glVertexAttribPointer(PositionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(UV1ID,      2, GL_FLOAT, GL_FALSE, 0, offset1);
  glVertexAttribPointer(UV2ID,      2, GL_FLOAT, GL_FALSE, 0, offset2 );

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

void RenderFrame() {
  ZoneScoped; // NOLINT

  // Start the ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  { // Imgui
    RenderText();
  }
  ImGui::Render();

  //glEnable(GL_DEPTH_TEST);
  //glEnable(GL_CULL_FACE);

  glViewport(0, 0, g_windowWidth, g_windowHeight);
  glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT);

  const auto projection = g_camera.getProjectionMatrix();
  const auto view       = g_camera.getViewMatrix();
  const auto MVP = projection * view;

  static GLint location = glGetUniformLocation(g_Program, "uMVP");
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));

  RenderFloor();
  //RenderText();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RenderText() {
    ZoneScoped; // NOLINT

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(g_windowWidth/2, g_windowHeight));
    ImGui::Begin("Text", nullptr, ImGuiWindowFlags_NoBackground |
                                  ImGuiWindowFlags_NoTitleBar   |
                                  ImGuiWindowFlags_NoResize     |
                                  ImGuiWindowFlags_NoSavedSettings);
    if (g_displayHelp) {
        ImGui::Text("%s", R"(First person camera behavior
  Press W and S to move forwards and backwards
  Press A and D to strafe left and right
  Press E and Q to move up and down
  Move mouse to free look

Flight camera behavior
  Press W and S to move forwards and backwards
  Press A and D to yaw left and right
  Press E and Q to move up and down
  Move mouse to pitch and roll

Press M to enable/disable mouse smoothing
Press V to enable/disable vertical sync
Press + and - to change camera rotation speed
Press , and . to change mouse sensitivity
Press SPACE to toggle between flight and first person behavior
Press ALT and ENTER to toggle full screen
Press ESC to exit

Press H to hide help)");
    } else {
        Mouse &mouse = Mouse::instance();
        const auto output = fmt::format(
R"(FPS: {}
Multisample anti-aliasing: {} x
Anisotropic filtering: {} x
Vertical sync: {}

Camera
  Position:
 x: {}
 y: {}
 z: {}
  Velocity:
 x: {}
 y: {}
 z: {}
  Rotation speed: {}
  Behavior: {}

Mouse
  Smoothing: {}
  Sensitivity: {}

Press H to display help)",
          g_framesPerSecond,
          g_msaaSamples,
          g_maxAnisotrophy,
          (g_enableVerticalSync ? "enabled" : "disabled"),
          g_camera.getPosition().x,
          g_camera.getPosition().y,
          g_camera.getPosition().z,
          g_camera.getCurrentVelocity().x,
          g_camera.getCurrentVelocity().y,
          g_camera.getCurrentVelocity().z,
          g_cameraRotationSpeed,
          (g_flightModeEnabled ? "Flight" : "First person"),
          (mouse.isMouseSmoothing() ? "enabled" : "disabled"),
          mouse.weightModifier()
);
        ImGui::TextColored(ImVec4(1.0F, 1.0F, 0.0F, 1.0F), "%s", output.c_str());
    }
    ImGui::End();
    //g_font.begin();
    //g_font.setColor(1.0f, 1.0f, 0.0f);
    //g_font.drawText(1, 1, output.str().c_str());
    //g_font.end();
}

void SetProcessorAffinity() {
    ZoneScoped; // NOLINT

    // Assign the current thread to one processor. This ensures that timing
    // code runs on only one processor, and will not suffer any ill effects
    // from power management.
    //
    // Based on DXUTSetProcessorAffinity() function from the DXUT framework.

    DWORD_PTR dwProcessAffinityMask = 0;
    DWORD_PTR dwSystemAffinityMask = 0;
    HANDLE hCurrentProcess = GetCurrentProcess();

    if (!GetProcessAffinityMask(hCurrentProcess, &dwProcessAffinityMask, &dwSystemAffinityMask))
        return;

    if (dwProcessAffinityMask)
    {
        // Find the lowest processor that our process is allowed to run against.

        DWORD_PTR dwAffinityMask = (dwProcessAffinityMask & ((~dwProcessAffinityMask) + 1));

        // Set this as the processor that our thread must always run against.
        // This must be a subset of the process affinity mask.

        HANDLE hCurrentThread = GetCurrentThread();

        if (hCurrentThread != INVALID_HANDLE_VALUE)
        {
            SetThreadAffinityMask(hCurrentThread, dwAffinityMask);
            CloseHandle(hCurrentThread);
        }
    }

    CloseHandle(hCurrentProcess);
}

void ToggleFullScreen() {
    ZoneScoped; // NOLINT

    static DWORD savedExStyle;
    static DWORD savedStyle;
    static RECT rcSaved;

    g_isFullScreen = !g_isFullScreen;

    if (g_isFullScreen) {
        // Moving to full screen mode.

        savedExStyle = static_cast<DWORD>(GetWindowLong(g_hWnd, GWL_EXSTYLE));
        savedStyle   = static_cast<DWORD>(GetWindowLong(g_hWnd, GWL_STYLE));
        GetWindowRect(g_hWnd, &rcSaved);

        SetWindowLong(g_hWnd, GWL_EXSTYLE, 0);
        SetWindowLong(g_hWnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        g_windowWidth = GetSystemMetrics(SM_CXSCREEN);
        g_windowHeight = GetSystemMetrics(SM_CYSCREEN);

        SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0,
            g_windowWidth, g_windowHeight, SWP_SHOWWINDOW);
    } else {
        // Moving back to windowed mode.

        SetWindowLong(g_hWnd, GWL_EXSTYLE, static_cast<LONG>(savedExStyle));
        SetWindowLong(g_hWnd, GWL_STYLE,   static_cast<LONG>(savedStyle));
        SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        g_windowWidth = rcSaved.right - rcSaved.left;
        g_windowHeight = rcSaved.bottom - rcSaved.top;

        SetWindowPos(g_hWnd, HWND_NOTOPMOST, rcSaved.left, rcSaved.top,
            g_windowWidth, g_windowHeight, SWP_SHOWWINDOW);
    }

    g_camera.perspective(CAMERA_FOVX,
        static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight),
        CAMERA_ZNEAR, CAMERA_ZFAR);
}

void UpdateCamera(float elapsedTimeSec) {
    ZoneScoped; // NOLINT

    float heading = 0.0f;
    float pitch   = 0.0f;
    float roll    = 0.0f;
    glm::vec3 direction;
    Mouse &mouse = Mouse::instance();

    GetMovementDirection(direction);

    switch (g_camera.getBehavior()) {
    case Camera::CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON:
        pitch   =  mouse.yDistanceFromWindowCenter() * g_cameraRotationSpeed;
        heading = -mouse.xDistanceFromWindowCenter() * g_cameraRotationSpeed;

        g_camera.rotate(heading, pitch, 0.0f);
        break;

    case Camera::CameraBehavior::CAMERA_BEHAVIOR_FLIGHT:
        heading = -direction.x * CAMERA_SPEED_FLIGHT_YAW * elapsedTimeSec;
        pitch   = -mouse.yDistanceFromWindowCenter() * g_cameraRotationSpeed;
        roll    = -mouse.xDistanceFromWindowCenter() * g_cameraRotationSpeed;

        g_camera.rotate(heading, pitch, roll);
        direction.x = 0.0f; // ignore yaw motion when updating camera velocity
        break;
    }

    g_camera.updatePosition(direction, elapsedTimeSec);
    PerformCameraCollisionDetection();

    mouse.moveToWindowCenter();
}

void UpdateFrame(float elapsedTimeSec) {
    ZoneScoped; // NOLINT

    UpdateFrameRate(elapsedTimeSec);

    Mouse::instance().update();
    Keyboard::instance().update();

    UpdateCamera(elapsedTimeSec);
    ProcessUserInput();
}

void UpdateFrameRate(float elapsedTimeSec) {
    ZoneScoped; // NOLINT

    static float accumTimeSec = 0.0f;
    static int frames = 0;

    accumTimeSec += elapsedTimeSec;

    if (accumTimeSec > 1.0f) {
        g_framesPerSecond = frames;

        frames = 0;
        accumTimeSec = 0.0f;
    } else {
        ++frames;
    }
}

void createBuffers() {
  ZoneScoped;  // NOLINT

  glGenVertexArrays(1, &g_VAO);
  glBindVertexArray(g_VAO);

  // clang-format off
  constexpr std::array<uint16_t, 6>  elements = {
      3, 1, 0,
      3, 2, 1
  };
  constexpr std::array<glm::vec3, 4> vertices = {
    glm::vec3(-FLOOR_WIDTH * 0.5F, 0.0F, FLOOR_HEIGHT * 0.5F),
    glm::vec3( FLOOR_WIDTH * 0.5F, 0.0F, FLOOR_HEIGHT * 0.5F),
    glm::vec3( FLOOR_WIDTH * 0.5F, 0.0F,-FLOOR_HEIGHT * 0.5F),
    glm::vec3(-FLOOR_WIDTH * 0.5F, 0.0F,-FLOOR_HEIGHT * 0.5F),
  };
  constexpr std::array<glm::vec2, 4> uvs0     = {
    glm::vec2(0.0F,         0.0F),
    glm::vec2(FLOOR_TILE_S, 0.0F),
    glm::vec2(FLOOR_TILE_S, FLOOR_TILE_T),
    glm::vec2(0.00F,        FLOOR_TILE_T),
  };
  constexpr std::array<glm::vec2, 4> uvs1     = {
    glm::vec2(0.0F, 0.0F),
    glm::vec2(1.0F, 0.0F),
    glm::vec2(1.0F, 1.0F),
    glm::vec2(0.0F, 1.0F),
  };

  constexpr auto verteicesSize = vertices.size() * sizeof(glm::vec3);
  constexpr auto uvs0Size      = uvs0.size()     * sizeof(glm::vec2);
  constexpr auto uvs1Size      = uvs1.size()     * sizeof(glm::vec2);
  constexpr auto totalSize     = verteicesSize + uvs0Size + uvs1Size;

  glCreateBuffers(1, &g_VBO);
  glNamedBufferStorage(g_VBO, totalSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
  glNamedBufferSubData(g_VBO, 0,                        verteicesSize, vertices.data());
  glNamedBufferSubData(g_VBO, verteicesSize,            uvs0Size, uvs0.data());
  glNamedBufferSubData(g_VBO, verteicesSize + uvs0Size, uvs1Size, uvs1.data());
  // clang-format on

  constexpr auto ElementsSize = static_cast<GLsizeiptr>(elements.size() * sizeof(uint16_t));
  glCreateBuffers(1, &g_EBO);
  glNamedBufferStorage(g_EBO, ElementsSize, elements.data(), GL_DYNAMIC_STORAGE_BIT);
}

void createUniformBuffers() {
    glCreateBuffers(1, &g_UBO);
    glNamedBufferStorage(g_UBO, sizeof(glm::mat4), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void createProgram() {
  ZoneScoped;  // NOLINT

  constexpr std::string_view VertexShader   = R"(
  #version 460 core

  layout(location=0) in vec3 aPosition;
  layout(location=1) in vec2 aUV0;
  layout(location=2) in vec2 aUV1;

  layout(location=0) uniform mat4 uMVP;

  out Interpolants {
    vec2 wUV0;
    vec2 wUV1;
  } OUT;

  void main() {
    OUT.wUV0 = aUV0;
    OUT.wUV1 = aUV1;
    gl_Position = uMVP * vec4(aPosition, 1);
  }
  )";
  constexpr std::string_view FragmentShader = R"(
  #version 460 core

  in Interpolants {
    vec2 wUV0;
    vec2 wUV1;
  } IN;

  layout(binding=1) uniform sampler2D uTexture0;
  layout(binding=2) uniform sampler2D uTexture1;

  layout(location=0) out vec4 out_Color;

  void main() {
    out_Color = texture(uTexture0, IN.wUV0) * texture(uTexture1, IN.wUV1);
  }
  )";

  const auto vertexShader = Shaders::createShader(GL_VERTEX_SHADER,   VertexShader.data());
  if(vertexShader == -1) {
      std::exit(EXIT_FAILURE);
  }

  const auto fragmentShader = Shaders::createShader(GL_FRAGMENT_SHADER, FragmentShader.data());
  if(fragmentShader == -1) {
    std::exit(EXIT_FAILURE);
  }

  const auto program = Shaders::createProgram(vertexShader, fragmentShader);
  if(program == -1) {
    std::exit(EXIT_FAILURE);
  }
  glDeleteShader(static_cast<GLuint>(vertexShader));
  glDeleteShader(static_cast<GLuint>(fragmentShader));

  g_Program = static_cast<GLuint>(program);

  g_uTexture0Locaion = glGetUniformLocation(g_Program, "uTexture0");
  if(g_uTexture0Locaion == -1) {
    throw std::runtime_error("\"uTexture0\" uniform doesn't exists.");
  }

  g_uTexture1Locaion = glGetUniformLocation(g_Program, "uTexture1");
  if(g_uTexture1Locaion == -1) {
    throw std::runtime_error("\"uTexture0\" uniform doesn't exists.");
  }
}
