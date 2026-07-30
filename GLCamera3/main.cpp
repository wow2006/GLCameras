//-----------------------------------------------------------------------------
// Copyright (c) 2007-2008 dhpoware. All rights reserved.
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
// This is the third demo in the OpenGL camera demo series. It extends the
// quaternion-based camera from GLCamera2 with spectator and orbit behaviors,
// and renders a 3D OBJ model with Blinn-Phong lighting.
//
//-----------------------------------------------------------------------------
// stb
#include <stb_image.h>
// SDL2
#include <SDL2/SDL.h>
//
#include "camera.hpp"
#include "input.hpp"
#include "model_obj.hpp"
#include "shaders.hpp"

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------

namespace {
constexpr auto APP_TITLE = "OpenGL Camera Demo 3";

constexpr float FLOOR_WIDTH = 8.0F;
constexpr float FLOOR_HEIGHT = 8.0F;
constexpr float FLOOR_TILE_S = 8.0F;
constexpr float FLOOR_TILE_T = 8.0F;

constexpr float CAMERA_FOVX = 90.0F;
constexpr float CAMERA_ZFAR = 100.0F;
constexpr float CAMERA_ZNEAR = 0.1F;
constexpr float CAMERA_ZOOM_MAX = 5.0F;
constexpr float CAMERA_ZOOM_MIN = 1.5F;

constexpr float CAMERA_SPEED_FLIGHT_YAW = 100.0F;
constexpr float CAMERA_SPEED_ORBIT_ROLL = 100.0F;

constexpr glm::vec3 CAMERA_ACCELERATION(4.0F, 4.0F, 4.0F);
constexpr glm::vec3 CAMERA_VELOCITY(1.0F, 1.0F, 1.0F);

constexpr uint32_t MATRICES_BINDING_POINT = 0;
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
bool g_modelLoaded;
GLuint g_floorColorMapTexture;
GLuint g_floorLightMapTexture;
Camera g_camera;
glm::quat g_meshOrientation;
glm::vec3 g_meshPosition;
ModelOBJ g_model;
glm::vec3 g_cameraBoundsMax;
glm::vec3 g_cameraBoundsMin;
SDL_Window *g_pWindow = nullptr;
SDL_GLContext g_glcontext = nullptr;

GLuint g_floorVAO = 0;
GLuint g_floorVBO = 0;
GLuint g_floorEBO = 0;
GLuint g_UBO = 0;
GLuint g_floorProgram = 0;

GLint g_uFloorTexture0Location;
GLint g_uFloorTexture1Location;

GLuint g_modelVAO = 0;
GLuint g_modelVBO = 0;
GLuint g_modelEBO = 0;
GLuint g_modelProgram = 0;

GLint g_uModelMVPLocation;
GLint g_uModelMatrixLocation;
GLint g_uNormalMatrixLocation;
GLint g_uLightPosLocation;
GLint g_uCameraPosLocation;
GLint g_uMatAmbientLocation;
GLint g_uMatDiffuseLocation;
GLint g_uMatSpecularLocation;
GLint g_uMatShininessLocation;

//-----------------------------------------------------------------------------
// Functions Prototypes.
//-----------------------------------------------------------------------------

void ChangeCameraBehavior(Camera::CameraBehavior behavior);
void Cleanup();
void CleanupApp();
float GetElapsedTimeInSeconds();
void GetMovementDirection(glm::vec3 &direction);
bool Init();
void InitApp();
void InitCamera();
void InitGL();
void InitImgui();
void InitModel();
GLuint LoadTexture(const char *pszFilename);
GLuint LoadTexture(const char *pszFilename, GLenum magFilter, GLenum minFilter, GLenum wrapS, GLenum wrapT);
void Log(const char *pszMessage);
void PerformCameraCollisionDetection();
void ProcessUserInput();
void RenderFloor();
void RenderFrame();
void RenderModel();
void RenderText();
void UpdateCamera(float elapsedTimeSec);
void UpdateFrame(float elapsedTimeSec);
void UpdateFrameRate(float elapsedTimeSec);
void ToggleFullScreen();
void createFloorBuffers();
void createModelBuffers();
void createUniformBuffers();
void createFloorProgram();
void createModelProgram();

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

  constexpr auto flags =
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MOUSE_GRABBED | SDL_WINDOW_MOUSE_CAPTURE;
  g_pWindow =
      SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_windowResolution.x, g_windowResolution.y, flags);
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

      UpdateFrame(GetElapsedTimeInSeconds());
      RenderFrame();
      SDL_GL_SwapWindow(g_pWindow);
    }
  }
  Cleanup();
  SDL_Quit();
  return EXIT_SUCCESS;
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

  if(keyboard.keyDown(SDL_SCANCODE_W)) {
    if(!moveForwardsPressed) {
      moveForwardsPressed = true;
      g_camera.setCurrentVelocity(velocity.x, velocity.y, 0.0F);
    }
    direction.z += 1.0F;
  } else {
    moveForwardsPressed = false;
  }

  if(keyboard.keyDown(SDL_SCANCODE_S)) {
    if(!moveBackwardsPressed) {
      moveBackwardsPressed = true;
      g_camera.setCurrentVelocity(velocity.x, velocity.y, 0.0F);
    }
    direction.z -= 1.0F;
  } else {
    moveBackwardsPressed = false;
  }

  if(keyboard.keyDown(SDL_SCANCODE_D)) {
    if(!moveRightPressed) {
      moveRightPressed = true;
      g_camera.setCurrentVelocity(0.0F, velocity.y, velocity.z);
    }
    direction.x += 1.0F;
  } else {
    moveRightPressed = false;
  }

  if(keyboard.keyDown(SDL_SCANCODE_A)) {
    if(!moveLeftPressed) {
      moveLeftPressed = true;
      g_camera.setCurrentVelocity(0.0F, velocity.y, velocity.z);
    }
    direction.x -= 1.0F;
  } else {
    moveLeftPressed = false;
  }

  if(keyboard.keyDown(SDL_SCANCODE_E)) {
    if(!moveUpPressed) {
      moveUpPressed = true;
      g_camera.setCurrentVelocity(velocity.x, 0.0F, velocity.z);
    }
    direction.y += 1.0F;
  } else {
    moveUpPressed = false;
  }

  if(keyboard.keyDown(SDL_SCANCODE_Q)) {
    if(!moveDownPressed) {
      moveDownPressed = true;
      g_camera.setCurrentVelocity(velocity.x, 0.0F, velocity.z);
    }
    direction.y -= 1.0F;
  } else {
    moveDownPressed = false;
  }
}

void ChangeCameraBehavior(Camera::CameraBehavior behavior) {
  if(g_camera.getBehavior() == behavior)
    return;

  if(behavior == Camera::CAMERA_BEHAVIOR_ORBIT) {
    g_meshPosition = g_camera.getPosition();
    g_meshOrientation = glm::inverse(g_camera.getOrientation());
  }

  g_camera.setBehavior(behavior);

  if(behavior == Camera::CAMERA_BEHAVIOR_ORBIT)
    g_camera.rotate(0.0F, -30.0F, 0.0F);
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
  InitModel();

  if(!(g_floorColorMapTexture = LoadTexture("floor_color_map.tga"))) {
    throw std::runtime_error("Failed to load texture: floor_color_map.tga");
  }

  if(!(g_floorLightMapTexture = LoadTexture("floor_light_map.tga"))) {
    throw std::runtime_error("Failed to load texture: floor_light_map.tga");
  }

  InitCamera();

  createFloorBuffers();
  createUniformBuffers();
  createFloorProgram();

  if(g_modelLoaded) {
    createModelBuffers();
    createModelProgram();
  }
}

void InitCamera() {
  g_camera.perspective(
      CAMERA_FOVX, static_cast<float>(g_windowResolution.x) / static_cast<float>(g_windowResolution.y), CAMERA_ZNEAR, CAMERA_ZFAR);

  float cameraOffset = g_modelLoaded ? g_model.getHeight() * 0.5F : 1.0F;

  g_camera.setPosition({0.0F, cameraOffset, 0.0F});
  g_camera.setOrbitMinZoom(CAMERA_ZOOM_MIN);
  g_camera.setOrbitMaxZoom(CAMERA_ZOOM_MAX);
  g_camera.setOrbitOffsetDistance(CAMERA_ZOOM_MIN + (CAMERA_ZOOM_MAX - CAMERA_ZOOM_MIN) * 0.3F);

  g_camera.setAcceleration(CAMERA_ACCELERATION);
  g_camera.setVelocity(CAMERA_VELOCITY);

  ChangeCameraBehavior(Camera::CAMERA_BEHAVIOR_ORBIT);

  Mouse::instance().hideCursor(true);
  Mouse::instance().moveToWindowCenter();

  g_cameraBoundsMax = {FLOOR_WIDTH / 2.0F, 4.0F, FLOOR_HEIGHT / 2.0F};
  g_cameraBoundsMin = {-FLOOR_WIDTH / 2.0F, cameraOffset, -FLOOR_HEIGHT / 2.0F};
}

void InitModel() {
  if(g_model.import("content/models/bigship1.obj")) {
    g_model.normalize();
    g_modelLoaded = true;
  } else {
    fmt::print(fg(fmt::color::yellow), "WARNING: bigship1.obj not found. Orbit mode will show no model.\n");
    g_modelLoaded = false;
  }
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

  glbinding::initialize([](const char *name) { return reinterpret_cast<glbinding::ProcAddress>(SDL_GL_GetProcAddress(name)); });

#ifdef OPENGL_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(
      [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {},
      nullptr);
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

void PerformCameraCollisionDetection() {
  if(g_camera.getBehavior() != Camera::CAMERA_BEHAVIOR_ORBIT) {
    const glm::vec3 &pos = g_camera.getPosition();
    glm::vec3 newPos(pos);

    if(pos.x > g_cameraBoundsMax.x)
      newPos.x = g_cameraBoundsMax.x;
    if(pos.x < g_cameraBoundsMin.x)
      newPos.x = g_cameraBoundsMin.x;
    if(pos.y > g_cameraBoundsMax.y)
      newPos.y = g_cameraBoundsMax.y;
    if(pos.y < g_cameraBoundsMin.y)
      newPos.y = g_cameraBoundsMin.y;
    if(pos.z > g_cameraBoundsMax.z)
      newPos.z = g_cameraBoundsMax.z;
    if(pos.z < g_cameraBoundsMin.z)
      newPos.z = g_cameraBoundsMin.z;

    g_camera.setPosition(newPos);
  }
}

void ProcessUserInput() {
  Keyboard &keyboard = Keyboard::instance();
  Mouse &mouse = Mouse::instance();

  if(keyboard.keyDown(SDL_SCANCODE_LALT) || keyboard.keyDown(SDL_SCANCODE_RALT)) {
    if(keyboard.keyPressed(SDL_SCANCODE_RETURN))
      ToggleFullScreen();
  }

  if(keyboard.keyPressed(SDL_SCANCODE_BACKSPACE)) {
    switch(g_camera.getBehavior()) {
    default: break;
    case Camera::CAMERA_BEHAVIOR_FLIGHT: g_camera.undoRoll(); break;
    case Camera::CAMERA_BEHAVIOR_ORBIT:
      if(!g_camera.preferTargetYAxisOrbiting())
        g_camera.undoRoll();
      break;
    }
  }

  if(keyboard.keyPressed(SDL_SCANCODE_SPACE)) {
    if(g_camera.getBehavior() == Camera::CAMERA_BEHAVIOR_ORBIT)
      g_camera.setPreferTargetYAxisOrbiting(!g_camera.preferTargetYAxisOrbiting());
  }

  if(keyboard.keyPressed(SDL_SCANCODE_1))
    ChangeCameraBehavior(Camera::CAMERA_BEHAVIOR_FIRST_PERSON);

  if(keyboard.keyPressed(SDL_SCANCODE_2))
    ChangeCameraBehavior(Camera::CAMERA_BEHAVIOR_SPECTATOR);

  if(keyboard.keyPressed(SDL_SCANCODE_3))
    ChangeCameraBehavior(Camera::CAMERA_BEHAVIOR_FLIGHT);

  if(keyboard.keyPressed(SDL_SCANCODE_4))
    ChangeCameraBehavior(Camera::CAMERA_BEHAVIOR_ORBIT);

  if(keyboard.keyPressed(SDL_SCANCODE_H))
    g_displayHelp = !g_displayHelp;

  if(keyboard.keyPressed(SDL_SCANCODE_EQUALS) || keyboard.keyPressed(SDL_SCANCODE_KP_PLUS)) {
    g_camera.setRotationSpeed(g_camera.getRotationSpeed() + 0.01F);
    if(g_camera.getRotationSpeed() > 1.0F)
      g_camera.setRotationSpeed(1.0F);
  }

  if(keyboard.keyPressed(SDL_SCANCODE_MINUS) || keyboard.keyPressed(SDL_SCANCODE_KP_MINUS)) {
    g_camera.setRotationSpeed(g_camera.getRotationSpeed() - 0.01F);
    if(g_camera.getRotationSpeed() <= 0.0F)
      g_camera.setRotationSpeed(0.01F);
  }

  if(keyboard.keyPressed(SDL_SCANCODE_PERIOD)) {
    mouse.setWeightModifier(mouse.weightModifier() + 0.1F);
    if(mouse.weightModifier() > 1.0F)
      mouse.setWeightModifier(1.0F);
  }

  if(keyboard.keyPressed(SDL_SCANCODE_COMMA)) {
    mouse.setWeightModifier(mouse.weightModifier() - 0.1F);
    if(mouse.weightModifier() < 0.0F)
      mouse.setWeightModifier(0.0F);
  }

  if(keyboard.keyPressed(SDL_SCANCODE_M))
    mouse.smoothMouse(!mouse.isMouseSmoothing());

  if(keyboard.keyPressed(SDL_SCANCODE_V)) {
    g_enableVerticalSync = !g_enableVerticalSync;
    SDL_GL_SetSwapInterval(g_enableVerticalSync ? 1 : 0);
  }
}

void RenderFloor() {
  glUseProgram(g_floorProgram);

  constexpr auto FloorTextureId = 0;
  glBindTextureUnit(FloorTextureId, g_floorColorMapTexture);
  glUniform1i(g_uFloorTexture0Location, FloorTextureId);

  constexpr auto FloorLightTextureId = 1;
  glBindTextureUnit(FloorLightTextureId, g_floorLightMapTexture);
  glUniform1i(g_uFloorTexture1Location, FloorLightTextureId);

  constexpr auto PositionID = 0;
  constexpr auto UV1ID = 1;
  constexpr auto UV2ID = 2;

  glBindVertexArray(g_floorVAO);
  glBindVertexBuffer(0, g_floorVBO, 0, sizeof(float) * 7);

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

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_floorEBO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

void RenderModel() {
  if(!g_modelLoaded)
    return;

  glUseProgram(g_modelProgram);

  glm::mat4 modelMatrix = glm::mat4_cast(g_meshOrientation);
  modelMatrix[3] = glm::vec4(g_meshPosition, 1.0F);

  const auto &projection = g_camera.getProjectionMatrix();
  const auto &view = g_camera.getViewMatrix();
  const auto modelMVP = projection * view * modelMatrix;

  glBindBuffer(GL_UNIFORM_BUFFER, g_UBO);
  glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING_POINT, g_UBO);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(modelMVP));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  glUniformMatrix4fv(g_uModelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

  glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
  glUniformMatrix3fv(g_uNormalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));

  const glm::vec3 &cameraPos = g_camera.getPosition();
  glUniform3f(g_uLightPosLocation, cameraPos.x, cameraPos.y, cameraPos.z);
  glUniform3f(g_uCameraPosLocation, cameraPos.x, cameraPos.y, cameraPos.z);

  glBindVertexArray(g_modelVAO);

  for(int i = 0; i < g_model.getNumberOfMeshes(); ++i) {
    const ModelOBJ::Mesh &mesh = g_model.getMesh(i);
    const ModelOBJ::Material &material = g_model.getMaterial(mesh.materialIndex);

    glUniform4fv(g_uMatAmbientLocation, 1, material.ambient);
    glUniform4fv(g_uMatDiffuseLocation, 1, material.diffuse);
    glUniform4fv(g_uMatSpecularLocation, 1, material.specular);
    glUniform1f(g_uMatShininessLocation, material.shininess * 128.0F);

    glDrawElements(GL_TRIANGLES, mesh.triangleCount * 3, GL_UNSIGNED_INT,
                   reinterpret_cast<const void *>(static_cast<uintptr_t>(mesh.startIndex) * sizeof(int)));
  }
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

  if(g_camera.getBehavior() == Camera::CAMERA_BEHAVIOR_ORBIT && g_modelLoaded) {
    RenderModel();
  }

  const auto &projection = g_camera.getProjectionMatrix();
  const auto &view = g_camera.getViewMatrix();
  const auto floorMVP = projection * view;

  glBindBuffer(GL_UNIFORM_BUFFER, g_UBO);
  glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING_POINT, g_UBO);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(floorMVP));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  RenderFloor();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RenderText() {
  std::ostringstream output;

  if(g_displayHelp) {
    output << "Press 1 to switch to first person behavior" << std::endl
           << "Press 2 to switch to spectator behavior" << std::endl
           << "Press 3 to switch to flight behavior" << std::endl
           << "Press 4 to switch to orbit behavior" << std::endl
           << std::endl
           << "First Person and Spectator behaviors" << std::endl
           << "  Press W and S to move forwards and backwards" << std::endl
           << "  Press A and D to strafe left and right" << std::endl
           << "  Press E and Q to move up and down" << std::endl
           << "  Move mouse to free look" << std::endl
           << std::endl
           << "Flight behavior" << std::endl
           << "  Press W and S to move forwards and backwards" << std::endl
           << "  Press A and D to yaw left and right" << std::endl
           << "  Press E and Q to move up and down" << std::endl
           << "  Move mouse up and down to change pitch" << std::endl
           << "  Move mouse left and right to change roll" << std::endl
           << std::endl
           << "Orbit behavior" << std::endl
           << "  Press SPACE to enable/disable target Y axis orbiting" << std::endl
           << "  Move mouse to orbit the model" << std::endl
           << "  Mouse wheel to zoom in and out" << std::endl
           << std::endl
           << "Press M to enable/disable mouse smoothing" << std::endl
           << "Press V to enable/disable vertical sync" << std::endl
           << "Press + and - to change camera rotation speed" << std::endl
           << "Press , and . to change mouse sensitivity" << std::endl
           << "Press BACKSPACE or middle mouse button to level camera" << std::endl
           << "Press ALT and ENTER to toggle full screen" << std::endl
           << "Press ESC to exit" << std::endl
           << std::endl
           << "Press H to hide help";
  } else {
    const char *pszCurrentBehavior = nullptr;
    const char *pszOrbitStyle = nullptr;
    const Mouse &mouse = Mouse::instance();

    switch(g_camera.getBehavior()) {
    case Camera::CAMERA_BEHAVIOR_FIRST_PERSON: pszCurrentBehavior = "First Person"; break;
    case Camera::CAMERA_BEHAVIOR_SPECTATOR: pszCurrentBehavior = "Spectator"; break;
    case Camera::CAMERA_BEHAVIOR_FLIGHT: pszCurrentBehavior = "Flight"; break;
    case Camera::CAMERA_BEHAVIOR_ORBIT: pszCurrentBehavior = "Orbit"; break;
    default: pszCurrentBehavior = "Unknown"; break;
    }

    if(g_camera.preferTargetYAxisOrbiting())
      pszOrbitStyle = "Target Y axis";
    else
      pszOrbitStyle = "Free";

    output.setf(std::ios::fixed, std::ios::floatfield);
    output << std::setprecision(2);

    output << "FPS: " << g_framesPerSecond << std::endl
           << "Multisample anti-aliasing: " << g_msaaSamples << "x" << std::endl
           << "Anisotropic filtering: " << g_maxAnisotrophy << "x" << std::endl
           << "Vertical sync: " << (g_enableVerticalSync ? "enabled" : "disabled") << std::endl
           << std::endl
           << "Camera" << std::endl
           << "  Position:" << " x:" << g_camera.getPosition().x << " y:" << g_camera.getPosition().y
           << " z:" << g_camera.getPosition().z << std::endl
           << "  Velocity:" << " x:" << g_camera.getCurrentVelocity().x << " y:" << g_camera.getCurrentVelocity().y
           << " z:" << g_camera.getCurrentVelocity().z << std::endl
           << "  Behavior: " << pszCurrentBehavior << std::endl
           << "  Rotation speed: " << g_camera.getRotationSpeed() << std::endl
           << "  Orbit style: " << pszOrbitStyle << std::endl
           << std::endl
           << "Mouse" << std::endl
           << "  Smoothing: " << (mouse.isMouseSmoothing() ? "enabled" : "disabled") << std::endl
           << "  Sensitivity: " << mouse.weightModifier() << std::endl
           << std::endl
           << "Press H to display help";
  }
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(g_windowResolution.x) / 2.0F, static_cast<float>(g_windowResolution.y)));
  ImGui::Begin("Text", nullptr,
               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
  ImGui::TextColored(ImVec4(1.0F, 1.0F, 0.0F, 1.0F), "%s", output.str().c_str());
  ImGui::End();
}

void UpdateCamera(float elapsedTimeSec) {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;
  glm::vec3 direction;
  Mouse &mouse = Mouse::instance();

  GetMovementDirection(direction);

  switch(g_camera.getBehavior()) {
  default: break;

  case Camera::CAMERA_BEHAVIOR_FIRST_PERSON:
  case Camera::CAMERA_BEHAVIOR_SPECTATOR:
    dx = -mouse.xDistanceFromWindowCenter();
    dy = -mouse.yDistanceFromWindowCenter();

    g_camera.rotateSmoothly(dx, dy, 0.0F);
    g_camera.updatePosition(direction, elapsedTimeSec);
    break;

  case Camera::CAMERA_BEHAVIOR_FLIGHT:
    dy = mouse.yDistanceFromWindowCenter();
    dz = -mouse.xDistanceFromWindowCenter();

    g_camera.rotateSmoothly(0.0F, dy, dz);

    if((dx = -direction.x * CAMERA_SPEED_FLIGHT_YAW * elapsedTimeSec) != 0.0F)
      g_camera.rotate(dx, 0.0F, 0.0F);

    direction.x = 0.0F;
    g_camera.updatePosition(direction, elapsedTimeSec);
    break;

  case Camera::CAMERA_BEHAVIOR_ORBIT:
    dx = mouse.xDistanceFromWindowCenter();
    dy = mouse.yDistanceFromWindowCenter();

    g_camera.rotateSmoothly(dx, dy, 0.0F);

    if(!g_camera.preferTargetYAxisOrbiting()) {
      if((dz = direction.x * CAMERA_SPEED_ORBIT_ROLL * elapsedTimeSec) != 0.0F)
        g_camera.rotate(0.0F, 0.0F, dz);
    }

    if((dz = -mouse.wheelPos()) != 0.0F)
      g_camera.zoom(dz, g_camera.getOrbitMinZoom(), g_camera.getOrbitMaxZoom());

    break;
  }

  mouse.moveToWindowCenter();

  if(mouse.buttonPressed(Mouse::BUTTON_MIDDLE))
    g_camera.undoRoll();

  PerformCameraCollisionDetection();
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

void ToggleFullScreen() {
  // TODO(Hussein): Implement me
}

void createFloorBuffers() {
  glGenVertexArrays(1, &g_floorVAO);
  glBindVertexArray(g_floorVAO);

  // clang-format off
  constexpr std::array<uint16_t, 6> elements = {
      3, 1, 0,
      3, 2, 1
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
}

void createModelBuffers() {
  glGenVertexArrays(1, &g_modelVAO);
  glBindVertexArray(g_modelVAO);

  const auto vertexCount = g_model.getNumberOfVertices();
  const auto vertexSize = g_model.getVertexSize();
  const auto *vertexData = g_model.getVertexBuffer();

  glCreateBuffers(1, &g_modelVBO);
  glNamedBufferStorage(g_modelVBO, static_cast<GLsizeiptr>(vertexCount) * vertexSize, vertexData, GL_DYNAMIC_STORAGE_BIT);

  const auto indexCount = g_model.getNumberOfIndices();
  const auto *indexData = g_model.getIndexBuffer();

  glCreateBuffers(1, &g_modelEBO);
  glNamedBufferStorage(g_modelEBO, static_cast<GLsizeiptr>(indexCount) * static_cast<GLsizeiptr>(sizeof(int)), indexData,
                       GL_DYNAMIC_STORAGE_BIT);

  constexpr auto PositionID = 0;
  constexpr auto TexCoordID = 1;
  constexpr auto NormalID = 2;

  glBindVertexBuffer(0, g_modelVBO, 0, vertexSize);

  glEnableVertexAttribArray(PositionID);
  glVertexAttribFormat(PositionID, 3, GL_FLOAT, GL_FALSE, offsetof(ModelOBJ::Vertex, position));
  glVertexAttribBinding(PositionID, 0);

  glEnableVertexAttribArray(TexCoordID);
  glVertexAttribFormat(TexCoordID, 2, GL_FLOAT, GL_FALSE, offsetof(ModelOBJ::Vertex, texCoord));
  glVertexAttribBinding(TexCoordID, 0);

  glEnableVertexAttribArray(NormalID);
  glVertexAttribFormat(NormalID, 3, GL_FLOAT, GL_FALSE, offsetof(ModelOBJ::Vertex, normal));
  glVertexAttribBinding(NormalID, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_modelEBO);
}

inline size_t uboAligned(size_t size) { return ((size + 255) / 256) * 256; }

void createUniformBuffers() {
  glCreateBuffers(1, &g_UBO);
  glNamedBufferStorage(g_UBO, uboAligned(sizeof(glm::mat4)), nullptr, GL_DYNAMIC_STORAGE_BIT);
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

  layout(binding=1) uniform sampler2D uTexture0;
  layout(binding=2) uniform sampler2D uTexture1;

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

void createModelProgram() {
  constexpr std::string_view VertexShader = R"(
  #version 460 core

  layout(location=0) in vec3 aPosition;
  layout(location=1) in vec2 aTexCoord;
  layout(location=2) in vec3 aNormal;

  layout(std140, binding=0) uniform Matrices
  {
      mat4 uMVP;
  };

  uniform mat4 uModel;
  uniform mat3 uNormalMatrix;

  out Interpolants {
    vec3 wPosition;
    vec3 wNormal;
  } OUT;

  void main() {
    OUT.wPosition = vec3(uModel * vec4(aPosition, 1.0));
    OUT.wNormal = normalize(uNormalMatrix * aNormal);
    gl_Position = uMVP * vec4(aPosition, 1.0);
  }
  )";
  constexpr std::string_view FragmentShader = R"(
  #version 460 core

  in Interpolants {
    vec3 wPosition;
    vec3 wNormal;
  } IN;

  uniform vec3 uLightPos;
  uniform vec3 uCameraPos;
  uniform vec4 uMatAmbient;
  uniform vec4 uMatDiffuse;
  uniform vec4 uMatSpecular;
  uniform float uMatShininess;

  layout(location=0) out vec4 out_Color;

  void main() {
    vec3 N = normalize(IN.wNormal);
    vec3 L = normalize(uLightPos - IN.wPosition);
    vec3 V = normalize(uCameraPos - IN.wPosition);
    vec3 H = normalize(L + V);

    vec3 ambient = uMatAmbient.rgb * 0.1;
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = uMatDiffuse.rgb * NdotL;

    float NdotH = max(dot(N, H), 0.0);
    float spec = (uMatShininess > 0.0) ? pow(NdotH, uMatShininess) : 0.0;
    vec3 specular = uMatSpecular.rgb * spec;

    out_Color = vec4(ambient + diffuse + specular, uMatDiffuse.a);
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

  g_modelProgram = static_cast<GLuint>(program);

  g_uModelMatrixLocation = glGetUniformLocation(g_modelProgram, "uModel");
  g_uNormalMatrixLocation = glGetUniformLocation(g_modelProgram, "uNormalMatrix");
  g_uLightPosLocation = glGetUniformLocation(g_modelProgram, "uLightPos");
  g_uCameraPosLocation = glGetUniformLocation(g_modelProgram, "uCameraPos");
  g_uMatAmbientLocation = glGetUniformLocation(g_modelProgram, "uMatAmbient");
  g_uMatDiffuseLocation = glGetUniformLocation(g_modelProgram, "uMatDiffuse");
  g_uMatSpecularLocation = glGetUniformLocation(g_modelProgram, "uMatSpecular");
  g_uMatShininessLocation = glGetUniformLocation(g_modelProgram, "uMatShininess");
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
  if(g_UBO) {
    glDeleteBuffers(1, &g_UBO);
    g_UBO = 0;
  }
  if(g_floorProgram) {
    glDeleteProgram(g_floorProgram);
    g_floorProgram = 0;
  }

  if(g_modelVAO) {
    glDeleteVertexArrays(1, &g_modelVAO);
    g_modelVAO = 0;
  }
  if(g_modelVBO) {
    glDeleteBuffers(1, &g_modelVBO);
    g_modelVBO = 0;
  }
  if(g_modelEBO) {
    glDeleteBuffers(1, &g_modelEBO);
    g_modelEBO = 0;
  }
  if(g_modelProgram) {
    glDeleteProgram(g_modelProgram);
    g_modelProgram = 0;
  }
}
