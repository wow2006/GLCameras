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

// GL_EXT_texture_filter_anisotropic
constexpr auto GL_TEXTURE_MAX_ANISOTROPY_EXT     = 0x84FE;
constexpr auto GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FF;

const glm::vec3 CAMERA_ACCELERATION(8.0F, 8.0F, 8.0F);
const float     CAMERA_FOVX = 90.0F;
const glm::vec3 CAMERA_POS(0.0F, 1.0F, 0.0F);
const float     CAMERA_SPEED_ROTATION   = 0.2F;
const float     CAMERA_SPEED_FLIGHT_YAW = 100.0F;
const glm::vec3 CAMERA_VELOCITY(2.0F, 2.0F, 2.0F);
const float     CAMERA_ZFAR  = 100.0F;
const float     CAMERA_ZNEAR = 0.1F;

const float FLOOR_WIDTH  = 16.0F;
const float FLOOR_HEIGHT = 16.0F;
const float FLOOR_TILE_S = 8.0F;
const float FLOOR_TILE_T = 8.0F;

//-----------------------------------------------------------------------------
// Globals.
//-----------------------------------------------------------------------------

static HWND      g_hWnd;
static HDC       g_hDC;
static HGLRC     g_hRC;
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
static GLuint    g_floorDisplayList;
//static GLFont    g_font;
static Camera    g_camera;
static glm::vec3 g_cameraBoundsMax;
static glm::vec3 g_cameraBoundsMin;
static float     g_cameraRotationSpeed = CAMERA_SPEED_ROTATION;

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
void    LimitFrameRate(float frameRateLimit, float frameTimeSeconds);
GLuint  LoadTexture(const char *pszFilename);
GLuint  LoadTexture(const char *pszFilename, GLint magFilter, GLint minFilter,
                    GLint wrapS, GLint wrapT);
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

//-----------------------------------------------------------------------------
// Functions.
//-----------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
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
                FrameMark;
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
            if (g_isFullScreen)
                ShowWindow(hWnd, SW_MINIMIZE);
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

void Cleanup()
{
    ZoneScoped; // NOLINT
    CleanupApp();

    if (g_hDC)
    {
        if (g_hRC)
        {
            wglMakeCurrent(g_hDC, 0);
            wglDeleteContext(g_hRC);
            g_hRC = 0;
        }

        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = 0;
    }
}

void CleanupApp()
{
    ZoneScoped; // NOLINT
    //g_font.destroy();

    if (g_floorColorMapTexture)
    {
        glDeleteTextures(1, &g_floorColorMapTexture);
        g_floorColorMapTexture = 0;
    }

    if (g_floorLightMapTexture)
    {
        glDeleteTextures(1, &g_floorLightMapTexture);
        g_floorLightMapTexture = 0;
    }

    if (g_floorDisplayList) {
        //glDeleteLists(g_floorDisplayList, 1);
        g_floorDisplayList = 0;
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

    if (!initialized)
    {
        initialized = true;
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&lastTime));
        timeScale = 1.0f / freq;
    }

    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&time));
    elapsedTimeSec = (time - lastTime) * timeScale;
    lastTime = time;

    if (fabsf(elapsedTimeSec - actualElapsedTimeSec) < 1.0f)
    {
        memmove(&frameTimes[1], frameTimes, sizeof(frameTimes) - sizeof(frameTimes[0]));
        frameTimes[0] = elapsedTimeSec;

        if (sampleCount < MAX_SAMPLE_COUNT)
            ++sampleCount;
    }

    actualElapsedTimeSec = 0.0f;

    for (int i = 0; i < sampleCount; ++i)
        actualElapsedTimeSec += frameTimes[i];

    if (sampleCount > 0)
        actualElapsedTimeSec /= sampleCount;

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

        //if (!ExtensionSupported("GL_ARB_multitexture"))
        //    throw std::runtime_error("Required extension not supported: GL_ARB_multitexture.");

        InitApp();
        return true;
    } catch (const std::exception &e) {
        std::ostringstream msg;

        msg << "Application initialization failed!" << std::endl << std::endl;
        msg << e.what();

        Log(msg.str().c_str());
        return false;
    }
}

void InitApp() {
    ZoneScoped; // NOLINT

    // Setup fonts.
    //if (!g_font.create("Arial", 10, GLFont::BOLD)) {
    //    throw std::runtime_error("Failed to create font.");
    //}

    // Load textures.

    if (!(g_floorColorMapTexture = LoadTexture("floor_color_map.jpg")))
        throw std::runtime_error("Failed to load texture: floor_color_map.jpg");

    if (!(g_floorLightMapTexture = LoadTexture("floor_light_map.jpg")))
        throw std::runtime_error("Failed to load texture: floor_light_map.jpg");

    // Setup camera.

    g_camera.perspective(CAMERA_FOVX,
        static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight),
        CAMERA_ZNEAR, CAMERA_ZFAR);

    g_camera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON);
    g_camera.setPosition(CAMERA_POS);
    g_camera.setAcceleration(CAMERA_ACCELERATION);
    g_camera.setVelocity(CAMERA_VELOCITY);

    g_cameraBoundsMax = {FLOOR_WIDTH / 2.0f, 4.0f, FLOOR_HEIGHT / 2.0f};
    g_cameraBoundsMin = {-FLOOR_WIDTH / 2.0f, CAMERA_POS.y, -FLOOR_HEIGHT / 2.0f};

    Mouse::instance().hideCursor(true);
    Mouse::instance().moveToWindowCenter();

    // Setup display list for the floor.

    //g_floorDisplayList = glGenLists(1);
    //glNewList(g_floorDisplayList, GL_COMPILE);
    //glBegin(GL_QUADS);
    //    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0f, 0.0f);
    //    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0f, 0.0f);
    //    glVertex3f(-FLOOR_WIDTH * 0.5f, 0.0f, FLOOR_HEIGHT * 0.5f);
    //
    //    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, FLOOR_TILE_S, 0.0f);
    //    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0f, 0.0f);
    //    glVertex3f(FLOOR_WIDTH * 0.5f, 0.0f, FLOOR_HEIGHT * 0.5f);
    //
    //    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, FLOOR_TILE_S, FLOOR_TILE_T);
    //    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0f, 1.0f);
    //    glVertex3f(FLOOR_WIDTH * 0.5f, 0.0f, -FLOOR_HEIGHT * 0.5f);
    //
    //    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.00f, FLOOR_TILE_T);
    //    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0f, 1.0f);
    //    glVertex3f(-FLOOR_WIDTH * 0.5f, 0.0f, -FLOOR_HEIGHT * 0.5f);
    //glEnd();
    //glEndList();
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

void InitGL() {
  ZoneScoped; // NOLINT

  if(!(g_hDC = GetDC(g_hWnd))) {
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
      WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
      WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
      WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
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
  if (!num_formats) {
      throw std::runtime_error("wglChoosePixelFormatARB() failed.");
  }

  PIXELFORMATDESCRIPTOR pfd;
  DescribePixelFormat(g_hDC, pixel_format, sizeof(pfd), &pfd);
  if(!SetPixelFormat(g_hDC, pixel_format, &pfd)) {
      throw std::runtime_error("SetPixelFormat() failed.");
  }

  constexpr auto WGL_CONTEXT_MAJOR_VERSION_ARB    = 0x2091;
  constexpr auto WGL_CONTEXT_MINOR_VERSION_ARB    = 0x2092;
  constexpr auto WGL_CONTEXT_PROFILE_MASK_ARB     = 0x9126;
  constexpr auto WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;

  // clang-format off
  // Specify that we want to create an OpenGL 3.3 core profile context
  constexpr int attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 6,
      WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
      0,
  };
  // clang-format on

  HGLRC context = wglCreateContextAttribsARB(g_hDC, 0, attribs);
  if(!context) {
    throw std::runtime_error("wglCreateContextAttribsARB() failed.");
  }

  if(!wglMakeCurrent(g_hDC, context)) {
    throw std::runtime_error("wglMakeCurrent() failed.");
  }

  if(gl3wInit() != GL3W_OK) {
    throw std::runtime_error("gl3wInit() failed.");
  }

  EnableVerticalSync(false);
}

GLuint LoadTexture(const char *pszFilename) {
    ZoneScoped; // NOLINT
    return LoadTexture(pszFilename, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR,
        GL_REPEAT, GL_REPEAT);
}

GLuint LoadTexture(const char *pszFilename, GLint magFilter, GLint minFilter,
                   GLint wrapS, GLint wrapT) {
    ZoneScoped; // NOLINT
    GLuint id = 0;
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    void *pImage = stbi_load(pszFilename, &width, &height, &channels, 4);

    if(pImage != nullptr) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

        if (g_maxAnisotrophy > 1) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                g_maxAnisotrophy);
        }

        //gluBuild2DMipmaps(GL_TEXTURE_2D, 4, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pImage);
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

        if (mouse.weightModifier() < 0.0f)
            mouse.setWeightModifier(0.0f);
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

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g_floorColorMapTexture);

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g_floorLightMapTexture);

  //glCallList(g_floorDisplayList);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
}

void RenderFrame() {
  ZoneScoped; // NOLINT

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  //glDisable(GL_LIGHTING);

  glViewport(0, 0, g_windowWidth, g_windowHeight);
  glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //glMatrixMode(GL_PROJECTION);
  //glLoadMatrixf(&g_camera.getProjectionMatrix()[0][0]);

  //glMatrixMode(GL_MODELVIEW);
  //glLoadMatrixf(&g_camera.getViewMatrix()[0][0]);

  RenderFloor();
  //RenderText();
}

void RenderText() {
    ZoneScoped; // NOLINT
    std::ostringstream output;

    if (g_displayHelp)
    {
        output
            << "First person camera behavior" << std::endl
            << "  Press W and S to move forwards and backwards" << std::endl
            << "  Press A and D to strafe left and right" << std::endl
            << "  Press E and Q to move up and down" << std::endl
            << "  Move mouse to free look" << std::endl
            << std::endl
            << "Flight camera behavior" << std::endl
            << "  Press W and S to move forwards and backwards" << std::endl
            << "  Press A and D to yaw left and right" << std::endl
            << "  Press E and Q to move up and down" << std::endl
            << "  Move mouse to pitch and roll" << std::endl
            << std::endl
            << "Press M to enable/disable mouse smoothing" << std::endl
            << "Press V to enable/disable vertical sync" << std::endl
            << "Press + and - to change camera rotation speed" << std::endl
            << "Press , and . to change mouse sensitivity" << std::endl
            << "Press SPACE to toggle between flight and first person behavior" << std::endl
            << "Press ALT and ENTER to toggle full screen" << std::endl
            << "Press ESC to exit" << std::endl
            << std::endl
            << "Press H to hide help";
    }
    else
    {
        Mouse &mouse = Mouse::instance();

        output.setf(std::ios::fixed, std::ios::floatfield);
        output << std::setprecision(2);

        output
            << "FPS: " << g_framesPerSecond << std::endl
            << "Multisample anti-aliasing: " << g_msaaSamples << "x" << std::endl
            << "Anisotropic filtering: " << g_maxAnisotrophy << "x" << std::endl
            << "Vertical sync: " << (g_enableVerticalSync ? "enabled" : "disabled") << std::endl
            << std::endl
            << "Camera" << std::endl
            << "  Position:"
            << " x:" << g_camera.getPosition().x
            << " y:" << g_camera.getPosition().y
            << " z:" << g_camera.getPosition().z << std::endl
            << "  Velocity:"
            << " x:" << g_camera.getCurrentVelocity().x
            << " y:" << g_camera.getCurrentVelocity().y
            << " z:" << g_camera.getCurrentVelocity().z << std::endl
            << "  Rotation speed: " << g_cameraRotationSpeed << std::endl
            << "  Behavior: " << (g_flightModeEnabled ? "Flight" : "First person") << std::endl
            << std::endl
            << "Mouse" << std::endl
            << "  Smoothing: " << (mouse.isMouseSmoothing() ? "enabled" : "disabled") << std::endl
            << "  Sensitivity: " << mouse.weightModifier() << std::endl
            << std::endl
            << "Press H to display help";
    }

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

    if (g_isFullScreen)
    {
        // Moving to full screen mode.

        savedExStyle = GetWindowLong(g_hWnd, GWL_EXSTYLE);
        savedStyle = GetWindowLong(g_hWnd, GWL_STYLE);
        GetWindowRect(g_hWnd, &rcSaved);

        SetWindowLong(g_hWnd, GWL_EXSTYLE, 0);
        SetWindowLong(g_hWnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        g_windowWidth = GetSystemMetrics(SM_CXSCREEN);
        g_windowHeight = GetSystemMetrics(SM_CYSCREEN);

        SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0,
            g_windowWidth, g_windowHeight, SWP_SHOWWINDOW);
    }
    else
    {
        // Moving back to windowed mode.

        SetWindowLong(g_hWnd, GWL_EXSTYLE, savedExStyle);
        SetWindowLong(g_hWnd, GWL_STYLE, savedStyle);
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
