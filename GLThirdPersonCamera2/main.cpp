//-----------------------------------------------------------------------------
// Copyright (c) 2008 dhpoware. All rights reserved.
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
// This is the fifth demo in the OpenGL camera demo series and it builds
// on the OpenGL Third Person Camera - Part 1 demo.
// (http://www.dhpoware.com/downloads/GLThirdPersonCamera1).
//
// In this demo we extend the third person camera model to include a spring
// system. The camera and the 3D model are attached to each other by a spring.
// As the user moves the 3D model around the scene the camera is dragged along
// with it. When the user stops moving the 3D model the camera slowly moves
// back to its resting position behind the 3D model.
//
// The purpose of the spring system in the third person camera is to smooth out
// the camera movement. If you disable the camera spring system in the demo the
// camera will instantly snap to its new position in response to the user's
// input.
//
// The behavior of the spring system is governed by the spring's stiffness. The
// less stiff the spring the more laggy the camera becomes. Increasing the
// spring's stiffness will make the camera less laggy. The correct spring
// stiffness will depend on your application. However very small stiffness
// values will cause the camera to lag too far behind the 3D model, and very
// large stiffness values will negate the spring damping effect on the camera.
//
//-----------------------------------------------------------------------------

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#include "GL_ARB_multitexture.h"
#include "WGL_ARB_multisample.h"
#include "bitmap.h"
#include "entity3d.h"
#include "gl_font.h"
#include "input.h"
#include "mathlib.h"
#include "third_person_camera.h"

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------

#define APP_TITLE "OpenGL Third Person Camera Demo 2"

// Windows Vista compositing support.
#if !defined(PFD_SUPPORT_COMPOSITION)
#define PFD_SUPPORT_COMPOSITION 0x00008000
#endif

// GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

const float BALL_FORWARD_SPEED = 120.0f;
const float BALL_HEADING_SPEED = 120.0f;
const float BALL_ROLLING_SPEED = 280.0f;
const float BALL_RADIUS = 20.0f;
const int   BALL_STACKS = 18;
const int   BALL_SLICES = 18;

const float FLOOR_WIDTH = 1024.0f;
const float FLOOR_HEIGHT = 1024.0f;
const float FLOOR_TILE_S = 4.0f;
const float FLOOR_TILE_T = 4.0f;

const float CAMERA_FOVX = 80.0f;
const float CAMERA_ZFAR = FLOOR_WIDTH * 2.0f;
const float CAMERA_ZNEAR = 1.0f;
const float CAMERA_MAX_SPRING_CONSTANT = 100.0f;
const float CAMERA_MIN_SPRING_CONSTANT = 1.0f;

//-----------------------------------------------------------------------------
// Globals.
//-----------------------------------------------------------------------------

HWND                g_hWnd;
HDC                 g_hDC;
HGLRC               g_hRC;
HINSTANCE           g_hInstance;
int                 g_framesPerSecond;
int                 g_windowWidth;
int                 g_windowHeight;
int                 g_msaaSamples;
int                 g_maxAnisotrophy;
GLuint              g_ballColorMapTexture;
GLuint              g_floorColorMapTexture;
GLuint              g_floorLightMapTexture;
GLuint              g_floorDisplayList;
bool                g_isFullScreen;
bool                g_hasFocus;
bool                g_enableVerticalSync;
bool                g_displayHelp;
GLUquadricObj      *g_pQuadricObj;
GLFont              g_font;
ThirdPersonCamera   g_camera;
Entity3D            g_ball;

//-----------------------------------------------------------------------------
// Functions Prototypes.
//-----------------------------------------------------------------------------

void    Cleanup();
void    CleanupApp();
float   ClipBallToFloor(const Entity3D &ball, float forwardSpeed, float elapsedTimeSec);
HWND    CreateAppWindow(const WNDCLASSEX &wcl, const char *pszTitle);
void    EnableVerticalSync(bool enableVerticalSync);
bool    ExtensionSupported(const char *pszExtensionName);
float   GetElapsedTimeInSeconds();
bool    Init();
void    InitApp();
void    InitGL();
GLuint  LoadTexture(const char *pszFilename);
GLuint  LoadTexture(const char *pszFilename, GLint magFilter, GLint minFilter,
                    GLint wrapS, GLint wrapT);
void    Log(const char *pszMessage);
void    ProcessUserInput();
void    RenderBall();
void    RenderFloor();
void    RenderFrame();
void    RenderText();
void    SetProcessorAffinity();
void    ToggleFullScreen();
void    UpdateBall(float elapsedTimeSec);
void    UpdateFrame(float elapsedTimeSec);
void    UpdateFrameRate(float elapsedTimeSec);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------
// Functions.
//-----------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#if defined _DEBUG
    _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

    MSG msg = {0};
    WNDCLASSEX wcl = {0};

    wcl.cbSize = sizeof(wcl);
    wcl.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcl.lpfnWndProc = WindowProc;
    wcl.cbClsExtra = 0;
    wcl.cbWndExtra = 0;
    wcl.hInstance = g_hInstance = hInstance;
    wcl.hIcon = LoadIcon(0, IDI_APPLICATION);
    wcl.hCursor = LoadCursor(0, IDC_ARROW);
    wcl.hbrBackground = 0;
    wcl.lpszMenuName = 0;
    wcl.lpszClassName = "GLWindowClass";
    wcl.hIconSm = 0;

    if (!RegisterClassEx(&wcl))
        return 0;

    g_hWnd = CreateAppWindow(wcl, APP_TITLE);

    if (g_hWnd)
    {
        SetProcessorAffinity();

        if (Init())
        {
            ShowWindow(g_hWnd, nShowCmd);
            UpdateWindow(g_hWnd);

            while (true)
            {
                while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                        break;

                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                if (msg.message == WM_QUIT)
                    break;

                if (g_hasFocus)
                {
                    UpdateFrame(GetElapsedTimeInSeconds());
                    RenderFrame();
                    SwapBuffers(g_hDC);
                }
                else
                {
                    WaitMessage();
                }
            }
        }

        Cleanup();
        UnregisterClass(wcl.lpszClassName, hInstance);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ACTIVATE:
        switch (wParam)
        {
        default:
            break;

        case WA_ACTIVE:
        case WA_CLICKACTIVE:
            g_hasFocus = true;
            break;

        case WA_INACTIVE:
            if (g_isFullScreen)
                ShowWindow(hWnd, SW_MINIMIZE);
            g_hasFocus = false;
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        g_windowWidth = static_cast<int>(LOWORD(lParam));
        g_windowHeight = static_cast<int>(HIWORD(lParam));
        break;

    default:
        Keyboard::instance().handleMsg(hWnd, msg, wParam, lParam);
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Cleanup()
{
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
    g_font.destroy();

    if (g_floorLightMapTexture)
    {
        glDeleteTextures(1, &g_floorLightMapTexture);
        g_floorLightMapTexture = 0;
    }

    if (g_floorColorMapTexture)
    {
        glDeleteTextures(1, &g_floorColorMapTexture);
        g_floorColorMapTexture = 0;
    }

    if (g_ballColorMapTexture)
    {
        glDeleteTextures(1, &g_ballColorMapTexture);
        g_ballColorMapTexture = 0;
    }

    if (g_floorDisplayList)
    {
        glDeleteLists(g_floorDisplayList, 1);
        g_floorDisplayList = 0;
    }

    if (g_pQuadricObj)
    {
        gluDeleteQuadric(g_pQuadricObj);
        g_pQuadricObj = 0;
    }
}

float ClipBallToFloor(const Entity3D &ball, float forwardSpeed, float elapsedTimeSec)
{
    // Perform very simple collision detection to prevent the ball from
    // moving beyond the edges of the floor. Notice that we are predicting
    // whether the ball will move beyond the edges of the floor based on the
    // ball's current forward velocity and the amount of time that has elapsed.

    float floorBoundaryZ = FLOOR_HEIGHT * 0.5f - BALL_RADIUS;
    float floorBoundaryX = FLOOR_WIDTH * 0.5f - BALL_RADIUS;
    float velocity = forwardSpeed * elapsedTimeSec;
    Vector3 newBallPos = ball.getPosition() + ball.getForwardVector() * velocity;

    if (newBallPos.z > -floorBoundaryZ && newBallPos.z < floorBoundaryZ)
    {
        if (newBallPos.x > -floorBoundaryX && newBallPos.x < floorBoundaryX)
            return forwardSpeed; // ball will still be within floor's bounds
    }

    return 0.0f; // ball will be outside of floor's bounds...so stop the ball
}

HWND CreateAppWindow(const WNDCLASSEX &wcl, const char *pszTitle)
{
    // Create a window that is centered on the desktop. It's exactly 1/4 the
    // size of the desktop. Don't allow it to be resized.

    DWORD wndExStyle = WS_EX_OVERLAPPEDWINDOW;
    DWORD wndStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                     WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    HWND hWnd = CreateWindowEx(wndExStyle, wcl.lpszClassName, pszTitle,
                    wndStyle, 0, 0, 0, 0, 0, 0, wcl.hInstance, 0);

    if (hWnd)
    {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int halfScreenWidth = screenWidth / 2;
        int halfScreenHeight = screenHeight / 2;
        int left = (screenWidth - halfScreenWidth) / 2;
        int top = (screenHeight - halfScreenHeight) / 2;
        RECT rc = {0};

        SetRect(&rc, left, top, left + halfScreenWidth, top + halfScreenHeight);
        AdjustWindowRectEx(&rc, wndStyle, FALSE, wndExStyle);
        MoveWindow(hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

        GetClientRect(hWnd, &rc);
        g_windowWidth = rc.right - rc.left;
        g_windowHeight = rc.bottom - rc.top;
    }

    return hWnd;
}

void EnableVerticalSync(bool enableVerticalSync)
{
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

bool ExtensionSupported(const char *pszExtensionName)
{
    static const char *pszGLExtensions = 0;
    static const char *pszWGLExtensions = 0;

    if (!pszGLExtensions)
        pszGLExtensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

    if (!pszWGLExtensions)
    {
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

float GetElapsedTimeInSeconds()
{
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

bool Init()
{
    try
    {
        InitGL();

        if (!ExtensionSupported("GL_ARB_multitexture"))
            throw std::runtime_error("Required extension not supported: GL_ARB_multitexture.");

        InitApp();
        return true;
    }
    catch (const std::exception &e)
    {
        std::ostringstream msg;

        msg << "Application initialization failed!" << std::endl << std::endl;
        msg << e.what();

        Log(msg.str().c_str());
        return false;
    }
}

void InitApp()
{
    // Setup fonts.

    if (!g_font.create("Arial", 10, GLFont::BOLD))
        throw std::runtime_error("Failed to create font.");

    // Setup textures.

    if (!(g_ballColorMapTexture = LoadTexture("ball_color_map.jpg")))
        throw std::runtime_error("Failed to load texture: ball_color_map.jpg");

    if (!(g_floorColorMapTexture = LoadTexture("floor_color_map.jpg")))
        throw std::runtime_error("Failed to load texture: floor_color_map.jpg");

    if (!(g_floorLightMapTexture = LoadTexture("floor_light_map.jpg")))
        throw std::runtime_error("Failed to load texture: floor_light_map.jpg");

    // Setup display list for the floor.

    g_floorDisplayList = glGenLists(1);
    glNewList(g_floorDisplayList, GL_COMPILE);   
    glBegin(GL_QUADS);
        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0f, 0.0f);    
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0f, 0.0f);
        glVertex3f(-FLOOR_WIDTH * 0.5f, 0.0f, FLOOR_HEIGHT * 0.5f);

        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, FLOOR_TILE_S, 0.0f);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0f, 0.0f);
        glVertex3f(FLOOR_WIDTH * 0.5f, 0.0f, FLOOR_HEIGHT * 0.5f);

        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, FLOOR_TILE_S, FLOOR_TILE_T);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0f, 1.0f);
        glVertex3f(FLOOR_WIDTH * 0.5f, 0.0f, -FLOOR_HEIGHT * 0.5f);

        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.00f, FLOOR_TILE_T);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0f, 1.0f);
        glVertex3f(-FLOOR_WIDTH * 0.5f, 0.0f, -FLOOR_HEIGHT * 0.5f);
    glEnd();
    glEndList();

    // Initialize the quadric object used to create and render the ball.

    if (!(g_pQuadricObj = gluNewQuadric()))
        throw std::runtime_error("gluNewQuadric() failed.");

    gluQuadricTexture(g_pQuadricObj, GL_TRUE);
    gluQuadricNormals(g_pQuadricObj, GL_SMOOTH);
    gluQuadricOrientation(g_pQuadricObj, GLU_OUTSIDE);

    // Initialize the ball.

    g_ball.constrainToWorldYAxis(true);
    g_ball.setPosition(0.0f, 1.0f + BALL_RADIUS, 0.0f);

    // Setup the camera.

    g_camera.perspective(CAMERA_FOVX,
        static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight),
        CAMERA_ZNEAR, CAMERA_ZFAR);

    g_camera.lookAt(Vector3(0.0f, BALL_RADIUS * 3.0f, BALL_RADIUS * 7.0f),
        Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
}

void InitGL()
{
    if (!(g_hDC = GetDC(g_hWnd)))
        throw std::runtime_error("GetDC() failed.");

    int pf = 0;
    PIXELFORMATDESCRIPTOR pfd = {0};
    OSVERSIONINFO osvi = {0};

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&osvi))
        throw std::runtime_error("GetVersionEx() failed.");

    // When running under Windows Vista or later support desktop composition.
    if (osvi.dwMajorVersion > 6 || (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 0))
        pfd.dwFlags |=  PFD_SUPPORT_COMPOSITION;

    ChooseBestMultiSampleAntiAliasingPixelFormat(pf, g_msaaSamples);

    if (!pf)
        pf = ChoosePixelFormat(g_hDC, &pfd);

    if (!SetPixelFormat(g_hDC, pf, &pfd))
        throw std::runtime_error("SetPixelFormat() failed.");

    if (!(g_hRC = wglCreateContext(g_hDC)))
        throw std::runtime_error("wglCreateContext() failed.");

    if (!wglMakeCurrent(g_hDC, g_hRC))
        throw std::runtime_error("wglMakeCurrent() failed.");

    EnableVerticalSync(false);

    // Check for GL_EXT_texture_filter_anisotropic support.
    if (ExtensionSupported("GL_EXT_texture_filter_anisotropic"))
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &g_maxAnisotrophy);
    else
        g_maxAnisotrophy = 1;
}

GLuint LoadTexture(const char *pszFilename)
{
    return LoadTexture(pszFilename, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR,
        GL_REPEAT, GL_REPEAT);
}

GLuint LoadTexture(const char *pszFilename, GLint magFilter, GLint minFilter,
                   GLint wrapS, GLint wrapT)
{
    GLuint id = 0;
    Bitmap bitmap;

    if (bitmap.loadPicture(pszFilename))
    {
        // The Bitmap class loads images and orients them top-down.
        // OpenGL expects bitmap images to be oriented bottom-up.
        bitmap.flipVertical();

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

        if (g_maxAnisotrophy > 1)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                g_maxAnisotrophy);
        }

        gluBuild2DMipmaps(GL_TEXTURE_2D, 4, bitmap.width, bitmap.height,
            GL_BGRA_EXT, GL_UNSIGNED_BYTE, bitmap.getPixels());
    }

    return id;
}

void Log(const char *pszMessage)
{
    MessageBox(0, pszMessage, "Error", MB_ICONSTOP);
}

void ProcessUserInput()
{
    Keyboard &keyboard = Keyboard::instance();

    if (keyboard.keyPressed(Keyboard::KEY_ESCAPE))
        PostMessage(g_hWnd, WM_CLOSE, 0, 0);

    if (keyboard.keyDown(Keyboard::KEY_LALT) || keyboard.keyDown(Keyboard::KEY_RALT))
    {
        if (keyboard.keyPressed(Keyboard::KEY_ENTER))
            ToggleFullScreen();
    }

    if (keyboard.keyPressed(Keyboard::KEY_H))
        g_displayHelp = !g_displayHelp;

    if (keyboard.keyPressed(Keyboard::KEY_V))
        EnableVerticalSync(!g_enableVerticalSync);

    if (keyboard.keyPressed(Keyboard::KEY_SPACE))
        g_camera.enableSpringSystem(!g_camera.springSystemIsEnabled());

    if (keyboard.keyPressed(Keyboard::KEY_ADD) || keyboard.keyPressed(Keyboard::KEY_NUMPAD_ADD))
    {
        float springConstant = g_camera.getSpringConstant() + 0.1f;

        springConstant = min(CAMERA_MAX_SPRING_CONSTANT, springConstant);
        g_camera.setSpringConstant(springConstant);
    }

    if (keyboard.keyPressed(Keyboard::KEY_SUBTRACT) || keyboard.keyPressed(Keyboard::KEY_NUMPAD_SUBTRACT))
    {
        float springConstant = g_camera.getSpringConstant() - 0.1f;

        springConstant = max(CAMERA_MIN_SPRING_CONSTANT, springConstant);
        g_camera.setSpringConstant(springConstant);
    }
}

void RenderBall()
{
    static float lightDir[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_ballColorMapTexture);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glPushMatrix();

    lightDir[0] = g_camera.getZAxis().x;
    lightDir[1] = g_camera.getZAxis().y;
    lightDir[2] = g_camera.getZAxis().z;

    glLightfv(GL_LIGHT0, GL_POSITION, lightDir);

    glMultMatrixf(&g_ball.getWorldMatrix()[0][0]);
    gluSphere(g_pQuadricObj, BALL_RADIUS, BALL_SLICES, BALL_STACKS);   
    glPopMatrix();

    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);    
}

void RenderFloor()
{
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_floorColorMapTexture);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_floorLightMapTexture);

    glCallList(g_floorDisplayList);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void RenderFrame()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glViewport(0, 0, g_windowWidth, g_windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&g_camera.getProjectionMatrix()[0][0]);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&g_camera.getViewMatrix()[0][0]);

    RenderBall();
    RenderFloor();
    RenderText();
}

void RenderText()
{
    std::ostringstream output;

    if (g_displayHelp)
    {
        output
            << "Press W or UP to roll the ball forwards" << std::endl
            << "Press S or DOWN to roll the ball backwards" << std::endl
            << "Press D or RIGHT to turn the ball to the right" << std::endl
            << "Press A or LEFT to turn the ball to the left" << std::endl
            << std::endl
            << "Press V to enable/disable vertical sync" << std::endl
            << "Press SPACE to enable and disable the camera's spring system" << std::endl
            << "Press + and - to change the camera's spring constant" << std::endl
            << "Press ALT and ENTER to toggle full screen" << std::endl
            << "Press ESC to exit" << std::endl
            << std::endl
            << "Press H to hide help";
    }
    else
    {
        bool springOn = g_camera.springSystemIsEnabled();
        float springConstant = g_camera.getSpringConstant();
        float dampingConstant = g_camera.getDampingConstant();

        output.setf(std::ios::fixed, std::ios::floatfield);
        output << std::setprecision(2);

        output
            << "FPS: " << g_framesPerSecond << std::endl
            << "Multisample anti-aliasing: " << g_msaaSamples << "x" << std::endl
            << "Anisotropic filtering: " << g_maxAnisotrophy << "x" << std::endl
            << "Vertical sync: " << (g_enableVerticalSync ? "enabled" : "disabled") << std::endl
            << std::endl
            << "Camera" << std::endl
            << "  Spring " << (springOn ? "enabled" : "disabled") << std::endl
            << "  Spring constant: " << springConstant << std::endl
            << "  Damping constant: " << dampingConstant << std::endl
            << std::endl
            << "Press H to display help";
    }

    g_font.begin();
    g_font.setColor(1.0f, 1.0f, 0.0f);
    g_font.drawText(1, 1, output.str().c_str());
    g_font.end();
}

void SetProcessorAffinity()
{
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

void ToggleFullScreen()
{
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

void UpdateBall(float elapsedTimeSec)
{
    Keyboard &keyboard = Keyboard::instance();
    float pitch = 0.0f;
    float heading = 0.0f;
    float forwardSpeed = 0.0f;

    if (keyboard.keyDown(Keyboard::KEY_W) || keyboard.keyDown(Keyboard::KEY_UP))
    {
        forwardSpeed = BALL_FORWARD_SPEED;
        pitch = -BALL_ROLLING_SPEED;
    }

    if (keyboard.keyDown(Keyboard::KEY_S) || keyboard.keyDown(Keyboard::KEY_DOWN))
    {
        forwardSpeed = -BALL_FORWARD_SPEED;
        pitch = BALL_ROLLING_SPEED;
    }

    if (keyboard.keyDown(Keyboard::KEY_D) || keyboard.keyDown(Keyboard::KEY_RIGHT))
        heading = -BALL_HEADING_SPEED;

    if (keyboard.keyDown(Keyboard::KEY_A) || keyboard.keyDown(Keyboard::KEY_LEFT))
        heading = BALL_HEADING_SPEED;

    // Prevent the ball from rolling off the edge of the floor.
    forwardSpeed = ClipBallToFloor(g_ball, forwardSpeed, elapsedTimeSec);

    // First move the ball.
    g_ball.setVelocity(0.0f, 0.0f, forwardSpeed);
    g_ball.orient(heading, 0.0f, 0.0f);
    g_ball.rotate(0.0f, pitch, 0.0f);
    g_ball.update(elapsedTimeSec);

    // Then move the camera based on where the ball has moved to.
    // When the ball is moving backwards rotations are inverted to match
    // the direction of travel. Consequently the camera's rotation needs to be
    // inverted as well.

    g_camera.rotate((forwardSpeed >= 0.0f) ? heading : -heading, 0.0f);
    g_camera.lookAt(g_ball.getPosition());
    g_camera.update(elapsedTimeSec);
}

void UpdateFrame(float elapsedTimeSec)
{
    Keyboard::instance().update();
    ProcessUserInput();

    UpdateFrameRate(elapsedTimeSec);
    UpdateBall(elapsedTimeSec);
}

void UpdateFrameRate(float elapsedTimeSec)
{
    static float accumTimeSec = 0.0f;
    static int frames = 0;

    accumTimeSec += elapsedTimeSec;

    if (accumTimeSec > 1.0f)
    {
        g_framesPerSecond = frames;

        frames = 0;
        accumTimeSec = 0.0f;
    }
    else
    {
        ++frames;
    }
}