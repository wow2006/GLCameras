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
// This is the third demo in the OpenGL camera demo series and it builds
// on the previous demo (http://www.dhpoware.com/downloads/GLCamera2.zip).
//
// In this demo the third person camera model is implemented. The third person
// camera model is also sometimes referred to as the chase camera model. The
// third person camera is different to the first person and flight simulation
// camera models in that the user does not explicitly control the third person
// camera.
//
// A third person camera is attached to an object in the game world. The third
// person camera is typically positioned at some distance behind and above this
// object. As the user moves and rotates this object about the game world, the
// third person camera will follow this object and reposition itself so that it
// is always oriented behind and above this object.
//
// The third person camera model in this demo does not implement any dampening
// or lag when the camera repositions itself in response to the target object
// being moved and rotated. This results in the camera snapping to its new
// position and orientation. To minimize this behavior time based movement and
// rotations are used. A future demo will address this problem.
//
//-----------------------------------------------------------------------------
// stb
#include <stb_image.h>
// SDL2
#include <SDL2/SDL.h>
//
#include "entity3d.hpp"
#include "input.hpp"
#include "shaders.hpp"
#include "third_person_camera.hpp"

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------

namespace {
constexpr auto APP_TITLE = "OpenGL Third Person Camera Demo 1";

constexpr float PI = 3.14159265358979323846F;

constexpr float BALL_FORWARD_SPEED = 60.0F;
constexpr float BALL_HEADING_SPEED = 60.0F;
constexpr float BALL_ROLLING_SPEED = 140.0F;
constexpr float BALL_RADIUS = 20.0F;
constexpr int BALL_STACKS = 18;
constexpr int BALL_SLICES = 18;

constexpr float FLOOR_WIDTH = 1024.0F;
constexpr float FLOOR_HEIGHT = 1024.0F;
constexpr float FLOOR_TILE_S = 4.0F;
constexpr float FLOOR_TILE_T = 4.0F;

constexpr float CAMERA_FOVX = 80.0F;
constexpr float CAMERA_ZFAR = FLOOR_WIDTH * 2.0F;
constexpr float CAMERA_ZNEAR = 1.0F;

constexpr uint32_t MATRICES_BINDING_POINT = 0;

struct BallVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
};
}  // namespace

//-----------------------------------------------------------------------------
// Globals.
//-----------------------------------------------------------------------------

int g_framesPerSecond;
glm::ivec2 g_windowResolution;
int g_msaaSamples;
int g_maxAnisotrophy;
bool g_isFullScreen;
bool g_hasFocus;
bool g_enableVerticalSync;
bool g_displayHelp;
GLuint g_ballColorMapTexture;
GLuint g_floorColorMapTexture;
GLuint g_floorLightMapTexture;
ThirdPersonCamera g_camera;
Entity3D g_ball;
SDL_Window *g_pWindow = nullptr;
SDL_GLContext g_glcontext = nullptr;

GLuint g_UBO = 0;

GLuint g_floorVAO = 0;
GLuint g_floorVBO = 0;
GLuint g_floorEBO = 0;
GLuint g_floorProgram = 0;

GLint g_uFloorTexture0Location;
GLint g_uFloorTexture1Location;

GLuint g_ballVAO = 0;
GLuint g_ballVBO = 0;
GLuint g_ballEBO = 0;
GLuint g_ballProgram = 0;
GLsizei g_ballIndexCount = 0;

GLint g_uBallTextureLocation;
GLint g_uBallNormalMatrixLocation;
GLint g_uBallLightDirLocation;

//-----------------------------------------------------------------------------
// Functions Prototypes.
//-----------------------------------------------------------------------------

void Cleanup();
void CleanupApp();
float ClipBallToFloor(const Entity3D &ball, float forwardSpeed, float elapsedTimeSec);
float GetElapsedTimeInSeconds();
bool Init();
void InitApp();
void InitCamera();
void InitGL();
void InitImgui();
GLuint LoadTexture(const char *pszFilename);
GLuint LoadTexture(const char *pszFilename, GLenum magFilter, GLenum minFilter, GLenum wrapS, GLenum wrapT);
void Log(const char *pszMessage);
void ProcessUserInput();
void RenderBall();
void RenderFloor();
void RenderFrame();
void RenderText();
void ToggleFullScreen();
void UpdateBall(float elapsedTimeSec);
void UpdateFrame(float elapsedTimeSec);
void UpdateFrameRate(float elapsedTimeSec);
void createBallBuffers();
void createFloorBuffers();
void createUniformBuffers();
void createBallProgram();
void createFloorProgram();

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

  SDL_DisplayMode mode{};
  if(0 == SDL_GetDisplayMode(0, 0, &mode)) {
    g_windowResolution = {mode.w / 2, mode.h / 2};
  } else {
    g_windowResolution = {800, 600};
  }

  // Unlike the first person demos this camera is driven entirely by the ball,
  // so the mouse is left alone: no grabbing, no recentering, cursor visible.
  constexpr auto flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
  g_pWindow = SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_windowResolution.x, g_windowResolution.y, flags);
  if(nullptr == g_pWindow) {
    fmt::print(stderr, fg(fmt::color::red), "ERROR: Can not create SDL Window\n");
    return EXIT_FAILURE;
  }

  SDL_SetThreadPriority(SDL_THREAD_PRIORITY_TIME_CRITICAL);

  if(Init()) {
    bool bRunning = true;
    while(bRunning) {
      SDL_Event event;
      while(SDL_PollEvent(&event)) {
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
            g_camera.perspective(
              CAMERA_FOVX, static_cast<float>(g_windowResolution.x) / static_cast<float>(g_windowResolution.y), CAMERA_ZNEAR, CAMERA_ZFAR);
            break;
          case SDL_WINDOWEVENT_CLOSE: bRunning = false; break;
          case SDL_WINDOWEVENT_ENTER:
          case SDL_WINDOWEVENT_FOCUS_GAINED: g_hasFocus = true; break;
          case SDL_WINDOWEVENT_LEAVE:
          case SDL_WINDOWEVENT_FOCUS_LOST: g_hasFocus = false; break;
          }
        }
      }

      UpdateFrame(GetElapsedTimeInSeconds());
      RenderFrame();
      SDL_GL_SwapWindow(g_pWindow);
    }
  }
  Cleanup();
  SDL_Quit();
  return EXIT_SUCCESS;
}

float ClipBallToFloor(const Entity3D &ball, float forwardSpeed, float elapsedTimeSec) {
  // Perform very simple collision detection to prevent the ball from
  // moving beyond the edges of the floor. Notice that we are predicting
  // whether the ball will move beyond the edges of the floor based on the
  // ball's current forward velocity and the amount of time that has elapsed.

  const float floorBoundaryZ = FLOOR_HEIGHT * 0.5F - BALL_RADIUS;
  const float floorBoundaryX = FLOOR_WIDTH * 0.5F - BALL_RADIUS;
  const float velocity = forwardSpeed * elapsedTimeSec;
  const glm::vec3 newBallPos = ball.getPosition() + ball.getForwardVector() * velocity;

  if(newBallPos.z > -floorBoundaryZ && newBallPos.z < floorBoundaryZ) {
    if(newBallPos.x > -floorBoundaryX && newBallPos.x < floorBoundaryX)
      return forwardSpeed;  // ball will still be within floor's bounds
  }

  return 0.0F;  // ball will be outside of floor's bounds...so stop the ball
}

float GetElapsedTimeInSeconds() {
  static uint64_t lastTick = 0;
  uint64_t currentTick = SDL_GetTicks64();

  if(lastTick == 0) {
    lastTick = currentTick;
    return 0.0F;
  }

  uint64_t elapsedTicks = currentTick - lastTick;
  lastTick = currentTick;

  return static_cast<float>(elapsedTicks) / 1000.0F;
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

void InitApp() {
  if(!(g_ballColorMapTexture = LoadTexture("ball_color_map.jpg"))) {
    throw std::runtime_error("Failed to load texture: ball_color_map.jpg");
  }

  if(!(g_floorColorMapTexture = LoadTexture("floor_color_map.jpg"))) {
    throw std::runtime_error("Failed to load texture: floor_color_map.jpg");
  }

  if(!(g_floorLightMapTexture = LoadTexture("floor_light_map.jpg"))) {
    throw std::runtime_error("Failed to load texture: floor_light_map.jpg");
  }

  // Initialize the ball.
  g_ball.constrainToWorldYAxis(true);
  g_ball.setPosition(0.0F, 1.0F + BALL_RADIUS, 0.0F);

  InitCamera();

  createUniformBuffers();
  createFloorBuffers();
  createFloorProgram();
  createBallBuffers();
  createBallProgram();
}

void InitCamera() {
  g_camera.perspective(CAMERA_FOVX, static_cast<float>(g_windowResolution.x) / static_cast<float>(g_windowResolution.y), CAMERA_ZNEAR, CAMERA_ZFAR);

  g_camera.lookAt({0.0F, BALL_RADIUS * 3.0F, BALL_RADIUS * 7.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
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
    Log("Failed to set minor version to 6");
  }

  g_glcontext = SDL_GL_CreateContext(g_pWindow);
  if(nullptr == g_glcontext) {
    throw std::runtime_error("Failed to create OpenGL Context");
  }
  SDL_GL_MakeCurrent(g_pWindow, g_glcontext);

  SDL_GL_SetSwapInterval(1);
  g_enableVerticalSync = true;

  glbinding::initialize([](const char *name) { return reinterpret_cast<glbinding::ProcAddress>(SDL_GL_GetProcAddress(name)); });

#ifdef OPENGL_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(
    [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {}, nullptr);
#endif

  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &g_maxAnisotrophy);

  InitImgui();
}

void InitImgui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  [[maybe_unused]] ImGuiIO &io = ImGui::GetIO();
  ImGui_ImplSDL2_InitForOpenGL(g_pWindow, g_glcontext);
  ImGui_ImplOpenGL3_Init();
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

void ProcessUserInput() {
  Keyboard &keyboard = Keyboard::instance();

  if(keyboard.keyDown(SDL_SCANCODE_LALT) || keyboard.keyDown(SDL_SCANCODE_RALT)) {
    if(keyboard.keyPressed(SDL_SCANCODE_RETURN))
      ToggleFullScreen();
  }

  if(keyboard.keyPressed(SDL_SCANCODE_H))
    g_displayHelp = !g_displayHelp;

  if(keyboard.keyPressed(SDL_SCANCODE_V)) {
    g_enableVerticalSync = !g_enableVerticalSync;
    SDL_GL_SetSwapInterval(g_enableVerticalSync ? 1 : 0);
  }
}

void RenderBall() {
  glUseProgram(g_ballProgram);

  const glm::mat4 &worldMatrix = g_ball.getWorldMatrix();

  const auto ballMVP = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * worldMatrix;

  glBindBuffer(GL_UNIFORM_BUFFER, g_UBO);
  glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING_POINT, g_UBO);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(ballMVP));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));
  glUniformMatrix3fv(g_uBallNormalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));

  // The original demo parked GL_LIGHT0 on the camera's z axis with w = 0, which
  // is a directional headlight rather than a positional light.
  const glm::vec3 &lightDir = g_camera.getZAxis();
  glUniform3f(g_uBallLightDirLocation, lightDir.x, lightDir.y, lightDir.z);

  constexpr auto BallTextureId = 0;
  glBindTextureUnit(BallTextureId, g_ballColorMapTexture);
  glUniform1i(g_uBallTextureLocation, BallTextureId);

  glBindVertexArray(g_ballVAO);
  glDrawElements(GL_TRIANGLES, g_ballIndexCount, GL_UNSIGNED_SHORT, nullptr);
}

void RenderFloor() {
  glUseProgram(g_floorProgram);

  const auto floorMVP = g_camera.getProjectionMatrix() * g_camera.getViewMatrix();

  glBindBuffer(GL_UNIFORM_BUFFER, g_UBO);
  glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING_POINT, g_UBO);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(floorMVP));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  constexpr auto FloorTextureId = 0;
  glBindTextureUnit(FloorTextureId, g_floorColorMapTexture);
  glUniform1i(g_uFloorTexture0Location, FloorTextureId);

  constexpr auto FloorLightTextureId = 1;
  glBindTextureUnit(FloorLightTextureId, g_floorLightMapTexture);
  glUniform1i(g_uFloorTexture1Location, FloorLightTextureId);

  glBindVertexArray(g_floorVAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

void RenderFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(g_pWindow);
  ImGui::NewFrame();

  { RenderText(); }
  ImGui::Render();

  glViewport(0, 0, g_windowResolution.x, g_windowResolution.y);
  glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  RenderBall();
  RenderFloor();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RenderText() {
  std::ostringstream output;

  if(g_displayHelp) {
    output << "Press W or UP to roll the ball forwards" << std::endl
           << "Press S or DOWN to roll the ball backwards" << std::endl
           << "Press D or RIGHT to turn the ball to the right" << std::endl
           << "Press A or LEFT to turn the ball to the left" << std::endl
           << std::endl
           << "Press V to enable/disable vertical sync" << std::endl
           << "Press ALT and ENTER to toggle full screen" << std::endl
           << "Press ESC to exit" << std::endl
           << std::endl
           << "Press H to hide help";
  } else {
    output << "FPS: " << g_framesPerSecond << std::endl
           << "Multisample anti-aliasing: " << g_msaaSamples << "x" << std::endl
           << "Anisotropic filtering: " << g_maxAnisotrophy << "x" << std::endl
           << "Vertical sync: " << (g_enableVerticalSync ? "enabled" : "disabled") << std::endl
           << std::endl
           << "Press H to display help";
  }

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(g_windowResolution.x) / 2.0F, static_cast<float>(g_windowResolution.y)));
  ImGui::Begin("Text", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
  ImGui::TextColored(ImVec4(1.0F, 1.0F, 0.0F, 1.0F), "%s", output.str().c_str());
  ImGui::End();
}

void ToggleFullScreen() {
  // TODO(Hussein): Implement me
}

void UpdateBall(float elapsedTimeSec) {
  Keyboard &keyboard = Keyboard::instance();
  float pitch = 0.0F;
  float heading = 0.0F;
  float forwardSpeed = 0.0F;

  if(keyboard.keyDown(SDL_SCANCODE_W) || keyboard.keyDown(SDL_SCANCODE_UP)) {
    forwardSpeed = BALL_FORWARD_SPEED;
    pitch = -BALL_ROLLING_SPEED;
  }

  if(keyboard.keyDown(SDL_SCANCODE_S) || keyboard.keyDown(SDL_SCANCODE_DOWN)) {
    forwardSpeed = -BALL_FORWARD_SPEED;
    pitch = BALL_ROLLING_SPEED;
  }

  if(keyboard.keyDown(SDL_SCANCODE_D) || keyboard.keyDown(SDL_SCANCODE_RIGHT))
    heading = -BALL_HEADING_SPEED;

  if(keyboard.keyDown(SDL_SCANCODE_A) || keyboard.keyDown(SDL_SCANCODE_LEFT))
    heading = BALL_HEADING_SPEED;

  // Prevent the ball from rolling off the edge of the floor.
  forwardSpeed = ClipBallToFloor(g_ball, forwardSpeed, elapsedTimeSec);

  // First move the ball.
  g_ball.setVelocity(0.0F, 0.0F, forwardSpeed);
  g_ball.orient(heading, 0.0F, 0.0F);
  g_ball.rotate(0.0F, pitch, 0.0F);
  g_ball.update(elapsedTimeSec);

  // Then move the camera based on where the ball has moved to.
  // When the ball is moving backwards rotations are inverted to match
  // the direction of travel. Consequently the camera's rotation needs to be
  // inverted as well.

  g_camera.rotate((forwardSpeed >= 0.0F) ? heading : -heading, 0.0F);
  g_camera.lookAt(g_ball.getPosition());
  g_camera.update(elapsedTimeSec);
}

void UpdateFrame(float elapsedTimeSec) {
  UpdateFrameRate(elapsedTimeSec);

  Keyboard::instance().update();

  ProcessUserInput();
  UpdateBall(elapsedTimeSec);
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

inline size_t uboAligned(size_t size) { return ((size + 255) / 256) * 256; }

void createUniformBuffers() {
  glCreateBuffers(1, &g_UBO);
  glNamedBufferStorage(g_UBO, uboAligned(sizeof(glm::mat4)), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void createFloorBuffers() {
  glGenVertexArrays(1, &g_floorVAO);
  glBindVertexArray(g_floorVAO);

  // clang-format off
  // Wound counter-clockwise as seen from above so the floor survives the
  // GL_CULL_FACE that RenderFrame() enables. This is the triangulation of the
  // original demo's GL_QUADS winding (v0, v1, v2, v3); the {3,1,0, 3,2,1} used
  // by the other demos faces the other way and is only visible from underneath.
  constexpr std::array<uint16_t, 6> elements = {
      0, 1, 2,
      0, 2, 3
  };
  constexpr std::array<float, 4 * 7> vertices = {
    -FLOOR_WIDTH * 0.5F, 0.0F, FLOOR_HEIGHT * 0.5F, 0.0F,         0.0F,         0.0F, 0.0F,
     FLOOR_WIDTH * 0.5F, 0.0F, FLOOR_HEIGHT * 0.5F, FLOOR_TILE_S, 0.0F,         1.0F, 0.0F,
     FLOOR_WIDTH * 0.5F, 0.0F,-FLOOR_HEIGHT * 0.5F, FLOOR_TILE_S, FLOOR_TILE_T, 1.0F, 1.0F,
    -FLOOR_WIDTH * 0.5F, 0.0F,-FLOOR_HEIGHT * 0.5F, 0.00F,        FLOOR_TILE_T, 0.0F, 1.0F,
  };
  // clang-format on

  constexpr auto verticesSize = vertices.size() * sizeof(float);

  glCreateBuffers(1, &g_floorVBO);
  glNamedBufferStorage(g_floorVBO, verticesSize, vertices.data(), GL_DYNAMIC_STORAGE_BIT);

  constexpr auto elementsSize = static_cast<GLsizeiptr>(elements.size() * sizeof(uint16_t));
  glCreateBuffers(1, &g_floorEBO);
  glNamedBufferStorage(g_floorEBO, elementsSize, elements.data(), GL_DYNAMIC_STORAGE_BIT);

  constexpr auto PositionID = 0;
  constexpr auto UV0ID = 1;
  constexpr auto UV1ID = 2;

  glBindVertexBuffer(0, g_floorVBO, 0, sizeof(float) * 7);

  glEnableVertexAttribArray(PositionID);
  glVertexAttribFormat(PositionID, 3, GL_FLOAT, GL_FALSE, 0);
  glVertexAttribBinding(PositionID, 0);

  glEnableVertexAttribArray(UV0ID);
  glVertexAttribFormat(UV0ID, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec3));
  glVertexAttribBinding(UV0ID, 0);

  glEnableVertexAttribArray(UV1ID);
  glVertexAttribFormat(UV1ID, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) + sizeof(glm::vec2));
  glVertexAttribBinding(UV1ID, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_floorEBO);
}

void createBallBuffers() {
  // gluSphere() is gone with GLU, so the UV sphere is generated by hand. Like
  // gluSphere the poles sit on the z axis and t runs from 0 at -z to 1 at +z.
  std::vector<BallVertex> vertices;
  std::vector<uint16_t> elements;

  const auto stackCount = static_cast<size_t>(BALL_STACKS);
  const auto sliceCount = static_cast<size_t>(BALL_SLICES);

  vertices.reserve((stackCount + 1) * (sliceCount + 1));
  elements.reserve(stackCount * sliceCount * 6);

  for(int stack = 0; stack <= BALL_STACKS; ++stack) {
    const float v = static_cast<float>(stack) / static_cast<float>(BALL_STACKS);
    const float phi = v * PI;
    const float cosPhi = std::cos(phi);
    const float sinPhi = std::sin(phi);

    for(int slice = 0; slice <= BALL_SLICES; ++slice) {
      const float u = static_cast<float>(slice) / static_cast<float>(BALL_SLICES);
      const float theta = u * 2.0F * PI;

      const glm::vec3 normal = {sinPhi * std::cos(theta), sinPhi * std::sin(theta), cosPhi};
      vertices.push_back({normal * BALL_RADIUS, normal, {u, 1.0F - v}});
    }
  }

  const auto verticesPerRow = BALL_SLICES + 1;

  for(int stack = 0; stack < BALL_STACKS; ++stack) {
    for(int slice = 0; slice < BALL_SLICES; ++slice) {
      const auto i0 = static_cast<uint16_t>((stack * verticesPerRow) + slice);
      const auto i1 = static_cast<uint16_t>(((stack + 1) * verticesPerRow) + slice);
      const auto i2 = static_cast<uint16_t>(((stack + 1) * verticesPerRow) + slice + 1);
      const auto i3 = static_cast<uint16_t>((stack * verticesPerRow) + slice + 1);

      elements.push_back(i0);
      elements.push_back(i1);
      elements.push_back(i2);

      elements.push_back(i0);
      elements.push_back(i2);
      elements.push_back(i3);
    }
  }

  g_ballIndexCount = static_cast<GLsizei>(elements.size());

  glGenVertexArrays(1, &g_ballVAO);
  glBindVertexArray(g_ballVAO);

  glCreateBuffers(1, &g_ballVBO);
  glNamedBufferStorage(g_ballVBO, static_cast<GLsizeiptr>(vertices.size() * sizeof(BallVertex)), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

  glCreateBuffers(1, &g_ballEBO);
  glNamedBufferStorage(g_ballEBO, static_cast<GLsizeiptr>(elements.size() * sizeof(uint16_t)), elements.data(), GL_DYNAMIC_STORAGE_BIT);

  constexpr auto PositionID = 0;
  constexpr auto NormalID = 1;
  constexpr auto TexCoordID = 2;

  glBindVertexBuffer(0, g_ballVBO, 0, sizeof(BallVertex));

  glEnableVertexAttribArray(PositionID);
  glVertexAttribFormat(PositionID, 3, GL_FLOAT, GL_FALSE, offsetof(BallVertex, position));
  glVertexAttribBinding(PositionID, 0);

  glEnableVertexAttribArray(NormalID);
  glVertexAttribFormat(NormalID, 3, GL_FLOAT, GL_FALSE, offsetof(BallVertex, normal));
  glVertexAttribBinding(NormalID, 0);

  glEnableVertexAttribArray(TexCoordID);
  glVertexAttribFormat(TexCoordID, 2, GL_FLOAT, GL_FALSE, offsetof(BallVertex, texCoord));
  glVertexAttribBinding(TexCoordID, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ballEBO);
}

void createFloorProgram() {
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

  uniform sampler2D uTexture0;
  uniform sampler2D uTexture1;

  layout(location=0) out vec4 out_Color;

  void main() {
    out_Color = texture(uTexture0, IN.wUV0) * texture(uTexture1, IN.wUV1);
  }
  )";

  const auto vertexShader = Shaders::createShader(GL_VERTEX_SHADER, VertexShader.data());
  if(vertexShader == -1)
    std::exit(EXIT_FAILURE);

  const auto fragmentShader = Shaders::createShader(GL_FRAGMENT_SHADER, FragmentShader.data());
  if(fragmentShader == -1)
    std::exit(EXIT_FAILURE);

  const auto program = Shaders::createProgram(vertexShader, fragmentShader);
  if(program == -1)
    std::exit(EXIT_FAILURE);

  glDeleteShader(static_cast<GLuint>(vertexShader));
  glDeleteShader(static_cast<GLuint>(fragmentShader));

  g_floorProgram = static_cast<GLuint>(program);

  g_uFloorTexture0Location = glGetUniformLocation(g_floorProgram, "uTexture0");
  g_uFloorTexture1Location = glGetUniformLocation(g_floorProgram, "uTexture1");
}

void createBallProgram() {
  constexpr std::string_view VertexShader = R"(
  #version 460 core

  layout(location=0) in vec3 aPosition;
  layout(location=1) in vec3 aNormal;
  layout(location=2) in vec2 aTexCoord;

  layout(std140, binding=0) uniform Matrices
  {
      mat4 uMVP;
  };

  uniform mat3 uNormalMatrix;

  out Interpolants {
    vec3 wNormal;
    vec2 wTexCoord;
  } OUT;

  void main() {
    OUT.wNormal = normalize(uNormalMatrix * aNormal);
    OUT.wTexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aPosition, 1.0);
  }
  )";
  constexpr std::string_view FragmentShader = R"(
  #version 460 core

  in Interpolants {
    vec3 wNormal;
    vec2 wTexCoord;
  } IN;

  uniform sampler2D uTexture;
  uniform vec3 uLightDir;

  layout(location=0) out vec4 out_Color;

  void main() {
    vec3 N = normalize(IN.wNormal);
    vec3 L = normalize(uLightDir);

    // Matches the fixed function default: a global ambient term plus a white
    // diffuse light, modulating the texture.
    float NdotL = max(dot(N, L), 0.0);
    vec3 lighting = vec3(0.2) + vec3(0.8) * NdotL;

    out_Color = vec4(texture(uTexture, IN.wTexCoord).rgb * lighting, 1.0);
  }
  )";

  const auto vertexShader = Shaders::createShader(GL_VERTEX_SHADER, VertexShader.data());
  if(vertexShader == -1)
    std::exit(EXIT_FAILURE);

  const auto fragmentShader = Shaders::createShader(GL_FRAGMENT_SHADER, FragmentShader.data());
  if(fragmentShader == -1)
    std::exit(EXIT_FAILURE);

  const auto program = Shaders::createProgram(vertexShader, fragmentShader);
  if(program == -1)
    std::exit(EXIT_FAILURE);

  glDeleteShader(static_cast<GLuint>(vertexShader));
  glDeleteShader(static_cast<GLuint>(fragmentShader));

  g_ballProgram = static_cast<GLuint>(program);

  g_uBallTextureLocation = glGetUniformLocation(g_ballProgram, "uTexture");
  g_uBallNormalMatrixLocation = glGetUniformLocation(g_ballProgram, "uNormalMatrix");
  g_uBallLightDirLocation = glGetUniformLocation(g_ballProgram, "uLightDir");
}

void Cleanup() {
  CleanupApp();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  if(nullptr != g_glcontext) {
    SDL_GL_DeleteContext(g_glcontext);
    g_glcontext = nullptr;
  }

  if(nullptr != g_pWindow) {
    SDL_DestroyWindow(g_pWindow);
    g_pWindow = nullptr;
  }
}

void CleanupApp() {
  if(g_ballColorMapTexture) {
    glDeleteTextures(1, &g_ballColorMapTexture);
    g_ballColorMapTexture = 0;
  }

  if(g_floorColorMapTexture) {
    glDeleteTextures(1, &g_floorColorMapTexture);
    g_floorColorMapTexture = 0;
  }

  if(g_floorLightMapTexture) {
    glDeleteTextures(1, &g_floorLightMapTexture);
    g_floorLightMapTexture = 0;
  }

  if(g_floorVAO) {
    glDeleteVertexArrays(1, &g_floorVAO);
    g_floorVAO = 0;
  }
  if(g_floorVBO) {
    glDeleteBuffers(1, &g_floorVBO);
    g_floorVBO = 0;
  }
  if(g_floorEBO) {
    glDeleteBuffers(1, &g_floorEBO);
    g_floorEBO = 0;
  }
  if(g_floorProgram) {
    glDeleteProgram(g_floorProgram);
    g_floorProgram = 0;
  }

  if(g_ballVAO) {
    glDeleteVertexArrays(1, &g_ballVAO);
    g_ballVAO = 0;
  }
  if(g_ballVBO) {
    glDeleteBuffers(1, &g_ballVBO);
    g_ballVBO = 0;
  }
  if(g_ballEBO) {
    glDeleteBuffers(1, &g_ballEBO);
    g_ballEBO = 0;
  }
  if(g_ballProgram) {
    glDeleteProgram(g_ballProgram);
    g_ballProgram = 0;
  }

  if(g_UBO) {
    glDeleteBuffers(1, &g_UBO);
    g_UBO = 0;
  }
}
