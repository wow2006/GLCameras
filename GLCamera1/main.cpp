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
#include <SDL_scancode.h>
#include <stb_image.h>
// SDL2
#include <SDL2/SDL.h>
// Internal
#include "input.hpp"
#include "camera.hpp"
#include "shaders.hpp"

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------

namespace {
constexpr auto APP_TITLE = "OpenGL Vector Camera Demo";

constexpr glm::vec3 CAMERA_ACCELERATION(8.0F, 8.0F, 8.0F);
constexpr float CAMERA_FOVX = 90.0F;
constexpr glm::vec3 CAMERA_POS(0.0F, 1.0F, 0.0F);
constexpr float CAMERA_SPEED_ROTATION = 0.2F;
constexpr float CAMERA_SPEED_FLIGHT_YAW = 100.0F;
constexpr glm::vec3 CAMERA_VELOCITY(2.0F, 2.0F, 2.0F);
constexpr float CAMERA_ZFAR = 100.0F;
constexpr float CAMERA_ZNEAR = 0.1F;

constexpr float FLOOR_WIDTH = 16.0F;
constexpr float FLOOR_HEIGHT = 16.0F;
constexpr float FLOOR_TILE_S = 8.0F;
constexpr float FLOOR_TILE_T = 8.0F;

constexpr uint32_t MATRICES_BINDING_POINT = 0;
}  // namespace

//-----------------------------------------------------------------------------
// Globals.
//-----------------------------------------------------------------------------

static int g_framesPerSecond;
glm::ivec2 g_windowResolution;
static int g_msaaSamples;
static int g_maxAnisotrophy;
static bool g_isFullScreen;
static bool g_hasFocus;
static bool g_enableVerticalSync;
static bool g_displayHelp = false;
static bool g_flightModeEnabled;
static GLuint g_floorColorMapTexture;
static GLuint g_floorLightMapTexture;
static Camera g_camera;
static glm::vec3 g_cameraBoundsMax;
static glm::vec3 g_cameraBoundsMin;
static float g_cameraRotationSpeed = CAMERA_SPEED_ROTATION;

static GLuint g_VAO = 0;
static GLuint g_VBO = 0;
static GLuint g_EBO = 0;
static GLuint g_UBO = 0;
static GLuint g_Program = 0;

static GLint g_uTexture0Locaion;
static GLint g_uTexture1Locaion;

SDL_Window *g_pWindow = nullptr;
SDL_GLContext g_glcontext = nullptr;

//-----------------------------------------------------------------------------
// Functions Prototypes.
//-----------------------------------------------------------------------------

// void EnableVerticalSync(bool enableVerticalSync);
float GetElapsedTimeInSeconds();
void GetMovementDirection(glm::vec3 &direction);
bool Init();
void InitApp();
void InitOpenglExtensions();
void InitGL();
void InitImgui();
GLuint LoadTexture(const char *pszFilename);
GLuint LoadTexture(const char *pszFilename, GLenum magFilter, GLenum minFilter, GLenum wrapS, GLenum wrapT);
void Log(const char *pszMessage);
void PerformCameraCollisionDetection();
void ProcessUserInput();
void RenderFloor();
void RenderFrame();
void RenderText();
void ToggleFullScreen();
void UpdateCamera(float elapsedTimeSec);
void UpdateFrame(float elapsedTimeSec);
void UpdateFrameRate(float elapsedTimeSec);
void createBuffers();
void createUniformBuffers();
void createProgram();
void Cleanup();
void CleanupApp();

//-----------------------------------------------------------------------------
// Functions.
//-----------------------------------------------------------------------------

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
#if defined(_WIN32) && defined(_DEBUG)
  _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
  if(0 != SDL_Init(SDL_INIT_VIDEO)) {
    fmt::print(stderr, fg(fmt::color::red), "ERROR: Can not initailize SDL: {}\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  // Get the screen resolution
  SDL_DisplayMode mode{};
  if(0 == SDL_GetDisplayMode(0, 0, &mode)) {
    g_windowResolution = {mode.w / 2, mode.h / 2};
  } else {
    // fallback window resolution will be 800x600
    g_windowResolution = {800, 600};
  }

  constexpr auto flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MOUSE_GRABBED | SDL_WINDOW_MOUSE_CAPTURE;
  g_pWindow = SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_windowResolution.x, g_windowResolution.y, flags);
  if(nullptr == g_pWindow) {
    fmt::print(stderr, fg(fmt::color::red), "ERROR: Can not create SDL Window\n");
    return EXIT_FAILURE;
  }

  SDL_SetThreadPriority(SDL_THREAD_PRIORITY_TIME_CRITICAL);

  if(Init()) {
    Mouse::instance().moveToWindowCenter();
    bool bRunning = true;
    while(bRunning) {
      SDL_Event event;
      while(SDL_PollEvent(&event) != 0) {
        if(SDL_QUIT == event.type) {
          bRunning = false;
        }
        if(SDL_KEYDOWN == event.type) {
          if(SDL_SCANCODE_ESCAPE == event.key.keysym.scancode) {
            bRunning = false;
          }
        }
        if(SDL_WINDOWEVENT == event.type) {
          const auto windowEvent = event.window;
          switch(windowEvent.event) {
          case SDL_WINDOWEVENT_RESIZED:
            g_windowResolution = {static_cast<int>(windowEvent.data1), static_cast<int>(windowEvent.data2)};
            break;
          case SDL_WINDOWEVENT_CLOSE: bRunning = false; break;
          case SDL_WINDOWEVENT_ENTER:
          case SDL_WINDOWEVENT_FOCUS_GAINED:
            Mouse::instance().attach(g_pWindow);
            g_hasFocus = true;
            break;
          case SDL_WINDOWEVENT_LEAVE:
          case SDL_WINDOWEVENT_FOCUS_LOST:
            Mouse::instance().detach();
            g_hasFocus = false;
            break;
          }
        }
      }

      //if(g_hasFocus) {
      UpdateFrame(GetElapsedTimeInSeconds());
      RenderFrame();
      SDL_GL_SwapWindow(g_pWindow);
      //} else {
      //  SDL_WaitEvent(&event);
      //}
    }
  }
  Cleanup();
  SDL_Quit();
  return EXIT_SUCCESS;
}

float GetElapsedTimeInSeconds() {
  static uint64_t lastTick = 0;
  uint64_t currentTick = SDL_GetTicks64();

  // First time call, no previous tick to compare
  if(lastTick == 0) {
    lastTick = currentTick;
    return 0.0F;
  }

  // Calculate elapsed time
  uint64_t elapsedTicks = currentTick - lastTick;
  lastTick = currentTick;

  return static_cast<float>(elapsedTicks) / 1000.0F;
}

void GetMovementDirection(glm::vec3 &direction) {
  static bool moveForwardsPressed = false;
  static bool moveBackwardsPressed = false;
  static bool moveRightPressed = false;
  static bool moveLeftPressed = false;
  static bool moveUpPressed = false;
  static bool moveDownPressed = false;

  glm::vec3 velocity = g_camera.getCurrentVelocity();
  Keyboard &keyboard = Keyboard::instance();

  direction = {0.0F, 0.0F, 0.0F};

  if(keyboard.keyDown(SDL_Scancode::SDL_SCANCODE_W)) {
    if(!moveForwardsPressed) {
      moveForwardsPressed = true;
      g_camera.setCurrentVelocity(velocity.x, velocity.y, 0.0F);
    }

    direction.z += 1.0F;
  } else {
    moveForwardsPressed = false;
  }

  if(keyboard.keyDown(SDL_Scancode::SDL_SCANCODE_S)) {
    if(!moveBackwardsPressed) {
      moveBackwardsPressed = true;
      g_camera.setCurrentVelocity(velocity.x, velocity.y, 0.0F);
    }

    direction.z -= 1.0F;
  } else {
    moveBackwardsPressed = false;
  }

  if(keyboard.keyDown(SDL_Scancode::SDL_SCANCODE_D)) {
    if(!moveRightPressed) {
      moveRightPressed = true;
      g_camera.setCurrentVelocity(0.0F, velocity.y, velocity.z);
    }

    direction.x += 1.0F;
  } else {
    moveRightPressed = false;
  }

  if(keyboard.keyDown(SDL_Scancode::SDL_SCANCODE_A)) {
    if(!moveLeftPressed) {
      moveLeftPressed = true;
      g_camera.setCurrentVelocity(0.0F, velocity.y, velocity.z);
    }

    direction.x -= 1.0F;
  } else {
    moveLeftPressed = false;
  }

  if(keyboard.keyDown(SDL_Scancode::SDL_SCANCODE_E)) {
    if(!moveUpPressed) {
      moveUpPressed = true;
      g_camera.setCurrentVelocity(velocity.x, 0.0F, velocity.z);
    }

    direction.y += 1.0F;
  } else {
    moveUpPressed = false;
  }

  if(keyboard.keyDown(SDL_Scancode::SDL_SCANCODE_Q)) {
    if(!moveDownPressed) {
      moveDownPressed = true;
      g_camera.setCurrentVelocity(velocity.x, 0.0F, velocity.z);
    }

    direction.y -= 1.0F;
  } else {
    moveDownPressed = false;
  }
}

bool Init() {

  try {
    InitGL();
    InitApp();
  } catch(const std::exception &e) {
    const auto errorMessage = fmt::format("Application initialization failed!\n\n{}", e.what());
    Log(errorMessage.c_str());
    return false;
  }

  return true;
}

void InitGL() {
  if(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8) < 0) {
    Log("Failed to set the Red size to 8");
  }
  if(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8) < 0) {
    Log("Failed to set the green size to 8");
  }
  if(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8) < 0) {
    Log("Failed to set the blue size to 8");
  }
  if(SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32) < 0) {
    Log("Failed to set the buffer size to 32");
  }
  if(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16) < 0) {
    Log("Failed to set the Depth size to 16");
  }
  if(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) < 0) {
    Log("Failed to set the DoubleBuffer");
  }

#ifdef OPENGL_DEBUG
  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG) < 0) {
    Log("Failed to set OpenGL debug flag");
  }
#endif
  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) < 0) {
    Log("Failed to set core context");
  }
  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4) < 0) {
    Log("Failed to set major version to 4");
  }
  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6) < 0) {
    Log("Failed to set major version to 6");
  }

  g_glcontext = SDL_GL_CreateContext(g_pWindow);
  if(nullptr == g_glcontext) {
    throw std::runtime_error("Failed to create OpenGL Context");
  }
  SDL_GL_MakeCurrent(g_pWindow, g_glcontext);

  SDL_GL_SetSwapInterval(1);

  glbinding::initialize([](const char *name) { return reinterpret_cast<glbinding::ProcAddress>(SDL_GL_GetProcAddress(name)); });

#ifdef OPENGL_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(
    [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {}, 0);
#endif

  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &g_maxAnisotrophy);

  InitImgui();
}

void InitImgui() {

  // Setup Dear ImGui binding
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  [[maybe_unused]] ImGuiIO &io = ImGui::GetIO();
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  ImGui_ImplSDL2_InitForOpenGL(g_pWindow, g_glcontext);
  ImGui_ImplOpenGL3_Init();
}

void InitApp() {

  // Load textures.
  g_floorColorMapTexture = LoadTexture("floor_color_map.jpg");
  if(g_floorColorMapTexture == 0U) {
    throw std::runtime_error("Failed to load texture: floor_color_map.jpg");
  }

  g_floorLightMapTexture = LoadTexture("floor_light_map.jpg");
  if(g_floorLightMapTexture == 0U) {
    throw std::runtime_error("Failed to load texture: floor_light_map.jpg");
  }

  // Setup camera.
  g_camera.perspective(CAMERA_FOVX, static_cast<float>(g_windowResolution.x) / static_cast<float>(g_windowResolution.y), CAMERA_ZNEAR, CAMERA_ZFAR);

  g_camera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON);
  g_camera.setPosition(CAMERA_POS);
  g_camera.setAcceleration(CAMERA_ACCELERATION);
  g_camera.setVelocity(CAMERA_VELOCITY);

  g_cameraBoundsMax = {FLOOR_WIDTH / 2.0F, 4.0F, FLOOR_HEIGHT / 2.0F};
  g_cameraBoundsMin = {-FLOOR_WIDTH / 2.0F, CAMERA_POS.y, -FLOOR_HEIGHT / 2.0F};

  // Mouse::instance().hideCursor(true);
  Mouse::instance().moveToWindowCenter();

  createBuffers();
  createUniformBuffers();
  createProgram();
}

GLuint LoadTexture(const char *pszFilename) {

  return LoadTexture(pszFilename, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
}

GLuint LoadTexture(const char *pszFilename, GLenum magFilter, GLenum minFilter, GLenum wrapS, GLenum wrapT) {

  GLuint id = 0;
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_set_flip_vertically_on_load(1);
  void *pImage = stbi_load(pszFilename, &width, &height, &channels, 4);

  if(pImage != nullptr) {
    glCreateTextures(GL_TEXTURE_2D, 1, &id);

    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, magFilter);
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, minFilter);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrapS);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrapT);

    glTextureStorage2D(id, 1, GL_RGBA8, width, height);
    glTextureSubImage2D(id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pImage);
    if(minFilter == GL_LINEAR_MIPMAP_LINEAR) {
      glGenerateTextureMipmap(id);
    }
    glTextureParameteri(id, GL_TEXTURE_MAX_ANISOTROPY, g_maxAnisotrophy);

    stbi_image_free(pImage);
  }

  return id;
}

void Log(const char *pszMessage) { fmt::print("{}\n", pszMessage); }

void PerformCameraCollisionDetection() {
  const glm::vec3 &pos = g_camera.getPosition();
  glm::vec3 newPos(pos);

  if(pos.x > g_cameraBoundsMax.x) {
    newPos.x = g_cameraBoundsMax.x;
  }

  if(pos.x < g_cameraBoundsMin.x) {
    newPos.x = g_cameraBoundsMin.x;
  }

  if(pos.y > g_cameraBoundsMax.y) {
    newPos.y = g_cameraBoundsMax.y;
  }

  if(pos.y < g_cameraBoundsMin.y) {
    newPos.y = g_cameraBoundsMin.y;
  }

  if(pos.z > g_cameraBoundsMax.z) {
    newPos.z = g_cameraBoundsMax.z;
  }

  if(pos.z < g_cameraBoundsMin.z) {
    newPos.z = g_cameraBoundsMin.z;
  }

  g_camera.setPosition(newPos);
}

void ProcessUserInput() {

  Keyboard &keyboard = Keyboard::instance();
  Mouse &mouse = Mouse::instance();

#if 0
  if(keyboard.keyPressed(Keyboard::KEY_ESCAPE)) {
    // PostMessage(g_hWnd, WM_CLOSE, 0, 0);

    if(keyboard.keyDown(Keyboard::KEY_LALT) || keyboard.keyDown(Keyboard::KEY_RALT)) {
      if(keyboard.keyPressed(Keyboard::KEY_ENTER)) {
        ToggleFullScreen();
      }
    }
  }
#endif

  if(keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_H)) {
    g_displayHelp = !g_displayHelp;
  }

  if(keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_M)) {
    mouse.smoothMouse(!mouse.isMouseSmoothing());
  }

  if(keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_V)) {
    // EnableVerticalSync(!g_enableVerticalSync);

    if(keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_EQUALS) || keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_KP_PLUS)) {
      g_cameraRotationSpeed += 0.01F;

      if(g_cameraRotationSpeed > 1.0F) {
        g_cameraRotationSpeed = 1.0F;
      }
    }
  }

  if(keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_MINUS) || keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_KP_MINUS)) {
    g_cameraRotationSpeed -= 0.01F;

    if(g_cameraRotationSpeed <= 0.0F) {
      g_cameraRotationSpeed = 0.01F;
    }
  }

  if(keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_PERIOD)) {
    mouse.setWeightModifier(mouse.weightModifier() + 0.1F);

    if(mouse.weightModifier() > 1.0F) {
      mouse.setWeightModifier(1.0F);
    }
  }

  if(keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_COMMA)) {
    mouse.setWeightModifier(mouse.weightModifier() - 0.1F);

    if(mouse.weightModifier() < 0.0F) {
      mouse.setWeightModifier(0.0F);
    }
  }

  if(keyboard.keyPressed(SDL_Scancode::SDL_SCANCODE_SPACE)) {
    g_flightModeEnabled = !g_flightModeEnabled;

    if(g_flightModeEnabled) {
      g_camera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_FLIGHT);
    } else {
      const glm::vec3 &cameraPos = g_camera.getPosition();

      g_camera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON);
      g_camera.setPosition(cameraPos.x, CAMERA_POS.y, cameraPos.z);
    }
  }
}

void RenderFloor() {

  glUseProgram(g_Program);

  constexpr auto FloorTextureId = 0;
  glBindTextureUnit(FloorTextureId, g_floorColorMapTexture);
  glUniform1i(g_uTexture0Locaion, FloorTextureId);

  constexpr auto FloorLightTextureId = 1;
  glBindTextureUnit(FloorLightTextureId, g_floorLightMapTexture);
  glUniform1i(g_uTexture1Locaion, FloorLightTextureId);

  constexpr auto PositionID = 0;
  constexpr auto UV1ID = 1;
  constexpr auto UV2ID = 2;

  glBindVertexBuffer(0, g_VBO, 0, sizeof(float) * 7);

  glEnableVertexAttribArray(PositionID);
  glEnableVertexAttribArray(UV1ID);
  glEnableVertexAttribArray(UV2ID);

  const auto offset1 = sizeof(glm::vec3);
  const auto offset2 = (sizeof(glm::vec3) + sizeof(glm::vec2));

  glVertexAttribFormat(PositionID, 3, GL_FLOAT, GL_FALSE, 0);
  glVertexAttribFormat(UV1ID, 2, GL_FLOAT, GL_FALSE, offset1);
  glVertexAttribFormat(UV2ID, 2, GL_FLOAT, GL_FALSE, offset2);

  glVertexAttribBinding(PositionID, 0);
  glVertexAttribBinding(UV1ID, 0);
  glVertexAttribBinding(UV2ID, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

void RenderFrame() {

  // Start the ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(g_pWindow);
  ImGui::NewFrame();

  {  // Imgui
    RenderText();
  }
  ImGui::Render();

  //glEnable(GL_DEPTH_TEST);
  //glEnable(GL_CULL_FACE);

  glViewport(0, 0, g_windowResolution.x, g_windowResolution.y);
  glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT);

  const auto projection = g_camera.getProjectionMatrix();
  const auto view = g_camera.getViewMatrix();
  const auto MVP = projection * view;

  glBindBuffer(GL_UNIFORM_BUFFER, g_UBO);
  glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING_POINT, g_UBO);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(MVP));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  RenderFloor();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RenderText() {

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(g_windowResolution.x) / 2.0F, static_cast<float>(g_windowResolution.y)));
  ImGui::Begin("Text", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
  if(g_displayHelp) {
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
      mouse.weightModifier());
    ImGui::TextColored(ImVec4(1.0F, 1.0F, 0.0F, 1.0F), "%s", output.c_str());
  }
  ImGui::End();
}

void ToggleFullScreen() {

  // static DWORD savedExStyle;
  // static DWORD savedStyle;
  // static RECT rcSaved;

  // g_isFullScreen = !g_isFullScreen;

  // if(g_isFullScreen) {
  //   // Moving to full screen mode.

  //   savedExStyle = static_cast<DWORD>(GetWindowLong(g_hWnd, GWL_EXSTYLE));
  //   savedStyle = static_cast<DWORD>(GetWindowLong(g_hWnd, GWL_STYLE));
  //   GetWindowRect(g_hWnd, &rcSaved);

  //   SetWindowLong(g_hWnd, GWL_EXSTYLE, 0);
  //   SetWindowLong(g_hWnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
  //   SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

  //   g_windowWidth = GetSystemMetrics(SM_CXSCREEN);
  //   g_windowHeight = GetSystemMetrics(SM_CYSCREEN);

  //   SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, g_windowWidth, g_windowHeight, SWP_SHOWWINDOW);
  // } else {
  //   // Moving back to windowed mode.

  //   SetWindowLong(g_hWnd, GWL_EXSTYLE, static_cast<LONG>(savedExStyle));
  //   SetWindowLong(g_hWnd, GWL_STYLE, static_cast<LONG>(savedStyle));
  //   SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

  //   g_windowWidth = rcSaved.right - rcSaved.left;
  //   g_windowHeight = rcSaved.bottom - rcSaved.top;

  //   SetWindowPos(g_hWnd, HWND_NOTOPMOST, rcSaved.left, rcSaved.top, g_windowWidth, g_windowHeight, SWP_SHOWWINDOW);
  // }

  // g_camera.perspective(CAMERA_FOVX, static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight), CAMERA_ZNEAR, CAMERA_ZFAR);
}

void UpdateCamera(float elapsedTimeSec) {

  float heading = 0.0F;
  float pitch = 0.0F;
  float roll = 0.0F;
  glm::vec3 direction;
  Mouse &mouse = Mouse::instance();

  GetMovementDirection(direction);

  switch(g_camera.getBehavior()) {
  case Camera::CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON:
    pitch = mouse.yDistanceFromWindowCenter() * g_cameraRotationSpeed;
    heading = -mouse.xDistanceFromWindowCenter() * g_cameraRotationSpeed;

    g_camera.rotate(heading, pitch, 0.0F);
    break;

  case Camera::CameraBehavior::CAMERA_BEHAVIOR_FLIGHT:
    heading = -direction.x * CAMERA_SPEED_FLIGHT_YAW * elapsedTimeSec;
    pitch = -mouse.yDistanceFromWindowCenter() * g_cameraRotationSpeed;
    roll = -mouse.xDistanceFromWindowCenter() * g_cameraRotationSpeed;

    g_camera.rotate(heading, pitch, roll);
    direction.x = 0.0F;  // ignore yaw motion when updating camera velocity
    break;
  }

  g_camera.updatePosition(direction, elapsedTimeSec);
  PerformCameraCollisionDetection();

  mouse.moveToWindowCenter();
}

void UpdateFrame(float elapsedTimeSec) {

  UpdateFrameRate(elapsedTimeSec);

  Mouse::instance().update();
  Keyboard::instance().update();

  UpdateCamera(elapsedTimeSec);
  ProcessUserInput();
}

void UpdateFrameRate(float elapsedTimeSec) {

  static float accumTimeSec = 0.0F;
  static int frames = 0;

  accumTimeSec += elapsedTimeSec;

  if(accumTimeSec > 1.0F) {
    g_framesPerSecond = frames;

    frames = 0;
    accumTimeSec = 0.0F;
  } else {
    ++frames;
  }
}

void createBuffers() {

  glGenVertexArrays(1, &g_VAO);
  glBindVertexArray(g_VAO);

  // clang-format off
  constexpr std::array<uint16_t, 6>  elements = {
      3, 1, 0,
      3, 2, 1
  };
  constexpr std::array<float, 4 * 7> vertices = {
    -FLOOR_WIDTH * 0.5F, 0.0F, FLOOR_HEIGHT * 0.5F, 0.0F,         0.0F,         0.0F, 0.0F,
     FLOOR_WIDTH * 0.5F, 0.0F, FLOOR_HEIGHT * 0.5F, FLOOR_TILE_S, 0.0F,         1.0F, 0.0F,
     FLOOR_WIDTH * 0.5F, 0.0F,-FLOOR_HEIGHT * 0.5F, FLOOR_TILE_S, FLOOR_TILE_T, 1.0F, 1.0F,
    -FLOOR_WIDTH * 0.5F, 0.0F,-FLOOR_HEIGHT * 0.5F, 0.00F,        FLOOR_TILE_T, 0.0F, 1.0F,
  };

  constexpr auto verteicesSize = vertices.size() * sizeof(float);

  glCreateBuffers(1, &g_VBO);
  glNamedBufferStorage(g_VBO, verteicesSize, vertices.data(), GL_DYNAMIC_STORAGE_BIT);
  // clang-format on

  constexpr auto ElementsSize = static_cast<GLsizeiptr>(elements.size() * sizeof(uint16_t));
  glCreateBuffers(1, &g_EBO);
  glNamedBufferStorage(g_EBO, ElementsSize, elements.data(), GL_DYNAMIC_STORAGE_BIT);
}

inline size_t uboAligned(size_t size) { return ((size + 255) / 256) * 256; }

void createUniformBuffers() {
  glCreateBuffers(1, &g_UBO);
  glNamedBufferStorage(g_UBO, uboAligned(sizeof(glm::mat4)), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void createProgram() {

  constexpr std::string_view VertexShader = R"(
  #version 460 core

  layout(location=0) in vec3 aPosition;
  layout(location=1) in vec2 aUV0;
  layout(location=2) in vec2 aUV1;

  layout(std140, binding=0) uniform Matrices
  {
      mat4 uMVP;
  };

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

  const auto vertexShader = Shaders::createShader(GL_VERTEX_SHADER, VertexShader.data());
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

void Cleanup() {
  CleanupApp();

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  if(nullptr == g_glcontext) {
    SDL_GL_DeleteContext(g_glcontext);
    g_glcontext = nullptr;
  }
  if(nullptr == g_pWindow) {
    SDL_DestroyWindow(g_pWindow);
    g_pWindow = nullptr;
  }
}

void CleanupApp() {

  glBindVertexArray(0);
  if(g_VAO != 0) {
    glDeleteVertexArrays(1, &g_VAO);
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  if(g_VBO != 0) {
    glDeleteBuffers(1, &g_VBO);
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  if(g_EBO != 0U) {
    glDeleteBuffers(1, &g_EBO);
  }

  glUseProgram(0);
  if(g_Program != 0U) {
    glDeleteProgram(g_Program);
  }

  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  if(g_UBO != 0) {
    glDeleteBuffers(1, &g_UBO);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  if(g_floorColorMapTexture != 0U) {
    glDeleteTextures(1, &g_floorColorMapTexture);
  }

  if(g_floorLightMapTexture != 0U) {
    glDeleteTextures(1, &g_floorLightMapTexture);
  }
}
