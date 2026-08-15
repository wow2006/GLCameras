///////////////////////////////////////////////////////////////////////////////
// main.cpp
// ========
// Maya camera demo
///////////////////////////////////////////////////////////////////////////////
//
// This demo shows an Autodesk Maya style viewport camera from two angles at
// once, borrowing the two viewport layout of the OrbitCamera demo. The left
// viewport is a third person view of the world with the camera itself drawn in
// it, together with the line to its center of interest and a translucent cone
// for its field of view (a box, when it is orthographic). The right viewport
// is what that camera actually sees.
//
// The Maya bindings all live in the RIGHT viewport:
//
//   Alt + LMB   tumble        F         frame the model
//   Alt + MMB   track         A         frame the whole scene
//   Alt + RMB   dolly         [ / ]     view undo / redo
//   wheel       dolly         1..6 / 0  orthographic views / perspective
//
// The left viewport keeps the OrbitCamera demo's own bindings for the observer
// camera: drag LMB to tumble it, RMB or the wheel to move in and out.
//
// Run with --self-test to exercise the camera math and exit without opening a
// window.
//-----------------------------------------------------------------------------
// STL
#include <utility>
// SDL2
#include <SDL2/SDL.h>
// glm
#include <glm/gtc/matrix_transform.hpp>
//
#include "input.hpp"
#include "model_obj.hpp"
#include "shaders.hpp"
#include "maya_camera.hpp"

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------

namespace {
constexpr auto APP_TITLE = "OpenGL Maya Camera Demo";

constexpr float GRID_SIZE = 10.0F;
constexpr float GRID_STEP = 1.0F;
constexpr float CAM_DIST = 5.0F;
constexpr float FOV_Y = 50.0F;
constexpr float NEAR_PLANE = 0.1F;
constexpr float FAR_PLANE = 1000.0F;

constexpr float FOV_MIN = 10.0F;
constexpr float FOV_MAX = 100.0F;
constexpr float FOV_DIST = 11.0F;

constexpr float ZOOM_SCALE = 0.5F;
constexpr float ANGLE_SCALE = 0.2F;
constexpr float MIN_DIST = 1.0F;
constexpr float MAX_DIST = 30.0F;

// what A frames: the grid, plus enough room for the model sitting on it
constexpr float SCENE_RADIUS = GRID_SIZE * 1.2F;

constexpr auto OBJ_MODEL = "data/debugger_small_5k.obj";
constexpr auto OBJ_CAM = "data/camera.obj";

constexpr float PANEL_WIDTH = 380.0F;

constexpr uint32_t MATRICES_BINDING_POINT = 0;

struct FlatVertex {
  glm::vec3 position;
  glm::vec4 color;
};

struct Material {
  glm::vec4 ambient;
  glm::vec4 diffuse;
  glm::vec4 specular;
  float shininess;
};

// The original ignored the materials in the OBJ files and drove both models
// from these two hard coded ones.
constexpr Material DEFAULT_MATERIAL = {{0.8F, 0.6F, 0.2F, 1.0F}, {1.0F, 0.9F, 0.2F, 1.0F}, {1.0F, 1.0F, 1.0F, 1.0F}, 128.0F};
constexpr Material CAMERA_MATERIAL = {{0.0F, 0.0F, 0.0F, 1.0F}, {0.9F, 0.9F, 0.9F, 1.0F}, {1.0F, 1.0F, 1.0F, 1.0F}, 256.0F};
}  // namespace

//-----------------------------------------------------------------------------
// Globals.
//-----------------------------------------------------------------------------

int g_framesPerSecond;
glm::ivec2 g_windowResolution;
bool g_hasFocus;
bool g_enableVerticalSync;
SDL_Window *g_pWindow = nullptr;
SDL_GLContext g_glcontext = nullptr;

MayaCamera g_cam1;  // the third person camera looking at the scene
MayaCamera g_cam2;  // the Maya camera being demonstrated

// mirrors of cam2, refreshed once per frame, for the panel and the drawing of
// the camera model
glm::vec3 g_cameraPosition;
glm::vec3 g_cameraTarget;
glm::mat4 g_cameraMatrix;

float g_fov = FOV_Y;
bool g_gridEnabled = true;
bool g_fovEnabled = true;

bool g_mouseLeftDown;
bool g_mouseRightDown;
glm::ivec2 g_mousePos;

// the Maya gesture in progress in the point of view viewport, if any
enum class Gesture { None, Tumble, Track, Dolly };
Gesture g_gesture = Gesture::None;

ModelOBJ g_objModel;
ModelOBJ g_objCam;
bool g_objLoaded;
std::string g_objError;

GLuint g_UBO = 0;

GLuint g_flatProgram = 0;

GLuint g_litProgram = 0;
GLint g_uLitModelView = -1;
GLint g_uLitNormalMatrix = -1;
GLint g_uLitAmbient = -1;
GLint g_uLitDiffuse = -1;
GLint g_uLitSpecular = -1;
GLint g_uLitShininess = -1;

GLuint g_gridVAO = 0;
GLuint g_gridVBO = 0;
GLsizei g_gridVertexCount = 0;

GLuint g_dynamicVAO = 0;
GLuint g_dynamicVBO = 0;

GLuint g_modelVAO = 0;
GLuint g_modelVBO = 0;
GLuint g_modelEBO = 0;

GLuint g_camVAO = 0;
GLuint g_camVBO = 0;
GLuint g_camEBO = 0;

// the 5 vertices of the field of view cone, apex first
std::array<glm::vec3, 5> g_fovVertices;

//-----------------------------------------------------------------------------
// Functions Prototypes.
//-----------------------------------------------------------------------------

void Cleanup();
void CleanupApp();
void ComputeFovVertices(float fov);
float GetElapsedTimeInSeconds();
bool Init();
void InitApp();
void InitGL();
void InitImgui();
bool LoadObjs();
void Log(const char *pszMessage);
void ProcessUserInput();
void RenderCameraModel(const glm::mat4 &projection, const glm::mat4 &view);
void RenderControls();
void RenderFocalLine(const glm::mat4 &mvp);
void RenderFocalPoint(const glm::mat4 &mvp);
void RenderFov(const glm::mat4 &mvp);
void RenderOrthoVolume(const glm::mat4 &mvp);
void RenderFrame();
void FrameModel();
void FrameScene();
void RenderGrid(const glm::mat4 &mvp);
void RenderModel(const glm::mat4 &projection, const glm::mat4 &view);
void RenderThirdPersonView(const glm::ivec4 &viewport);
void RenderPointOfView(const glm::ivec4 &viewport);
void ResetCamera();
void RotateCamera(int x, int y);
void SetMVP(const glm::mat4 &mvp);
void SyncCameraState();
void ToggleFullScreen();
void UpdateFrame(float elapsedTimeSec);
void UpdateFrameRate(float elapsedTimeSec);
void ZoomCameraDelta(float delta);
void createDynamicBuffers();
void createGridBuffers();
void createUniformBuffers();
void createFlatProgram();
void createLitProgram();
void uploadObj(const ModelOBJ &model, GLuint &vao, GLuint &vbo, GLuint &ebo);

//-----------------------------------------------------------------------------
// Functions.
//-----------------------------------------------------------------------------

namespace {
// the third person viewport, driven by plain drags, and the point of view
// viewport, driven by the Maya bindings
glm::ivec4 g_view1Rect = {0, 0, 0, 0};
glm::ivec4 g_view2Rect = {0, 0, 0, 0};

[[nodiscard]] bool inside(const glm::ivec4 &rect, const glm::ivec2 &pos) {
  return pos.x >= rect.x && pos.x < (rect.x + rect.z) && pos.y >= 0 && pos.y < g_windowResolution.y;
}

[[nodiscard]] bool insideView1(const glm::ivec2 &pos) { return inside(g_view1Rect, pos); }

[[nodiscard]] bool insideView2(const glm::ivec2 &pos) { return inside(g_view2Rect, pos); }

[[nodiscard]] bool altDown() { return (SDL_GetModState() & KMOD_ALT) != 0; }
}  // namespace

int main(int argc, char *argv[]) {
  // headless check of the camera math, see mayaCameraSelfTest()
  for(int i = 1; i < argc; ++i) {
    if(std::string_view(argv[i]) == "--self-test") {
      mayaCameraSelfTest();
      fmt::print("MayaCamera self test passed\n");
      return EXIT_SUCCESS;
    }
  }

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
    g_windowResolution = {1024, 600};
  }

  // The camera is driven by dragging in the third person view and by the
  // control panel, so the mouse is left alone: cursor visible, no recentering.
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
        ImGui_ImplSDL2_ProcessEvent(&event);

        const bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;

        if(SDL_QUIT == event.type) {
          bRunning = false;
        }
        if(SDL_KEYDOWN == event.type) {
          if(SDL_SCANCODE_ESCAPE == event.key.keysym.scancode) {
            bRunning = false;
          }
        }
        if(SDL_MOUSEBUTTONDOWN == event.type && !imguiWantsMouse) {
          g_mousePos = {event.button.x, event.button.y};

          // Maya reserves Alt + drag for navigation, so a gesture only starts
          // with Alt held, and one gesture is one undo step.
          if(insideView2(g_mousePos) && altDown()) {
            switch(event.button.button) {
            case SDL_BUTTON_LEFT: g_gesture = Gesture::Tumble; break;
            case SDL_BUTTON_MIDDLE: g_gesture = Gesture::Track; break;
            case SDL_BUTTON_RIGHT: g_gesture = Gesture::Dolly; break;
            default: break;
            }
            if(Gesture::None != g_gesture)
              g_cam2.pushUndo();
          } else if(insideView1(g_mousePos)) {
            if(SDL_BUTTON_LEFT == event.button.button)
              g_mouseLeftDown = true;
            else if(SDL_BUTTON_RIGHT == event.button.button)
              g_mouseRightDown = true;
          }
        }
        if(SDL_MOUSEBUTTONUP == event.type) {
          g_gesture = Gesture::None;
          if(SDL_BUTTON_LEFT == event.button.button)
            g_mouseLeftDown = false;
          else if(SDL_BUTTON_RIGHT == event.button.button)
            g_mouseRightDown = false;
        }
        if(SDL_MOUSEMOTION == event.type) {
          const glm::vec2 delta = {static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel)};

          switch(g_gesture) {
          case Gesture::Tumble: g_cam2.tumble(delta); break;
          case Gesture::Track: g_cam2.track(delta, g_view2Rect.w); break;
          case Gesture::Dolly: g_cam2.dolly(delta); break;
          case Gesture::None:
            if(g_mouseLeftDown)
              RotateCamera(event.motion.x, event.motion.y);
            else if(g_mouseRightDown)
              ZoomCameraDelta(static_cast<float>(event.motion.y - g_mousePos.y));
            break;
          }

          g_mousePos = {event.motion.x, event.motion.y};
        }
        if(SDL_MOUSEWHEEL == event.type && !imguiWantsMouse) {
          if(insideView2(g_mousePos)) {
            g_cam2.pushUndo();
            g_cam2.dollyTicks(static_cast<float>(event.wheel.y));
          } else if(insideView1(g_mousePos)) {
            ZoomCameraDelta(static_cast<float>(event.wheel.y));
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
  ResetCamera();
  ComputeFovVertices(g_fov);

  createUniformBuffers();
  createGridBuffers();
  createDynamicBuffers();
  createFlatProgram();
  createLitProgram();

  // The two OBJ models are not part of this repository. Everything else in the
  // demo still runs without them, so a failure here is reported rather than
  // fatal.
  if(!LoadObjs())
    Log(g_objError.c_str());
}

///////////////////////////////////////////////////////////////////////////////
// reset both cameras
///////////////////////////////////////////////////////////////////////////////
void ResetCamera() {
  // third person camera
  g_cam1.lookAt({CAM_DIST * 2.0F, CAM_DIST * 1.5F, CAM_DIST * 2.0F}, {0.0F, 0.0F, 0.0F});
  g_cam1.setNearFar(NEAR_PLANE, FAR_PLANE);
  g_cam1.setFieldOfView(FOV_Y);

  // camera object
  g_cam2.lookAt({0.0F, 0.0F, CAM_DIST}, {0.0F, 0.0F, 0.0F});
  g_cam2.setNearFar(NEAR_PLANE, FAR_PLANE);
  g_cam2.setOrthographic(false);
  g_cam2.setTumblePivot(MayaCamera::TumblePivot::CenterOfInterest);
  SyncCameraState();

  g_fov = FOV_Y;
  g_cam2.setFieldOfView(g_fov);
  ComputeFovVertices(g_fov);

  g_gridEnabled = true;
  g_fovEnabled = true;
}

///////////////////////////////////////////////////////////////////////////////
// mirror cam2's state into the values the control panel and the third person
// view read
///////////////////////////////////////////////////////////////////////////////
void SyncCameraState() {
  g_cameraPosition = g_cam2.getEye();
  g_cameraTarget = g_cam2.getPivot();
  g_cameraMatrix = g_cam2.getViewMatrix();
}

///////////////////////////////////////////////////////////////////////////////
// F and A: fit the model, or the whole scene, into the view
///////////////////////////////////////////////////////////////////////////////
void FrameModel() {
  if(!g_objLoaded) {
    FrameScene();
    return;
  }

  glm::vec3 center;
  g_objModel.getCenter(center.x, center.y, center.z);

  // the bounding box's half diagonal, the smallest sphere holding the box
  const glm::vec3 extents = {g_objModel.getWidth(), g_objModel.getHeight(), g_objModel.getLength()};
  g_cam2.pushUndo();
  g_cam2.frame(center, glm::length(extents) * 0.5F);
  g_cam2.setSelectionPivot(center);
}

void FrameScene() {
  g_cam2.pushUndo();
  g_cam2.frame({0.0F, 0.0F, 0.0F}, SCENE_RADIUS);
}

///////////////////////////////////////////////////////////////////////////////
// compute vertices for FOV
///////////////////////////////////////////////////////////////////////////////
void ComputeFovVertices(float fov) {
  const float halfFov = glm::radians(fov) * 0.5F;
  constexpr float ratio = 1.0F;

  const float x = std::tan(halfFov * ratio) * FOV_DIST;
  const float y = std::tan(halfFov) * FOV_DIST;

  g_fovVertices[0] = {0.0F, 0.0F, 0.0F};  // origin
  g_fovVertices[1] = {x, y, FOV_DIST};    // top-left
  g_fovVertices[2] = {-x, y, FOV_DIST};   // top-right
  g_fovVertices[3] = {x, -y, FOV_DIST};   // bottom-left
  g_fovVertices[4] = {-x, -y, FOV_DIST};  // bottom-right
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
  if(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) < 0) {
    Log("Failed to set the Depth size to 24");
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

  InitImgui();
}

void InitImgui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  [[maybe_unused]] ImGuiIO &io = ImGui::GetIO();
  ImGui_ImplSDL2_InitForOpenGL(g_pWindow, g_glcontext);
  ImGui_ImplOpenGL3_Init();
}

bool LoadObjs() {
  if(!g_objModel.import(OBJ_MODEL)) {
    g_objError = fmt::format("Failed to load {}. The demo runs without it.", OBJ_MODEL);
    return false;
  }

  if(!g_objCam.import(OBJ_CAM)) {
    g_objError = fmt::format("Failed to load {}. The demo runs without it.", OBJ_CAM);
    return false;
  }

  uploadObj(g_objModel, g_modelVAO, g_modelVBO, g_modelEBO);
  uploadObj(g_objCam, g_camVAO, g_camVBO, g_camEBO);

  g_objLoaded = true;
  return true;
}

void Log(const char *pszMessage) { fmt::print("{}\n", pszMessage); }

void ProcessUserInput() {
  Keyboard &keyboard = Keyboard::instance();

  const bool alt = keyboard.keyDown(SDL_SCANCODE_LALT) || keyboard.keyDown(SDL_SCANCODE_RALT);
  if(alt) {
    if(keyboard.keyPressed(SDL_SCANCODE_RETURN))
      ToggleFullScreen();
  }

  if(keyboard.keyPressed(SDL_SCANCODE_V)) {
    g_enableVerticalSync = !g_enableVerticalSync;
    SDL_GL_SetSwapInterval(g_enableVerticalSync ? 1 : 0);
  }

  if(keyboard.keyPressed(SDL_SCANCODE_R))
    ResetCamera();

  // the panel owns the keyboard while a slider or a field has it
  if(ImGui::GetIO().WantCaptureKeyboard || alt)
    return;

  if(keyboard.keyPressed(SDL_SCANCODE_F))
    FrameModel();

  if(keyboard.keyPressed(SDL_SCANCODE_A))
    FrameScene();

  if(keyboard.keyPressed(SDL_SCANCODE_LEFTBRACKET))
    g_cam2.undo();

  if(keyboard.keyPressed(SDL_SCANCODE_RIGHTBRACKET))
    g_cam2.redo();

  // the orthographic bookmarks, and back to the perspective camera on 0
  constexpr std::array<std::pair<SDL_Scancode, MayaCamera::StandardView>, 7> views = {{
    {SDL_SCANCODE_1, MayaCamera::StandardView::Front},
    {SDL_SCANCODE_2, MayaCamera::StandardView::Back},
    {SDL_SCANCODE_3, MayaCamera::StandardView::Left},
    {SDL_SCANCODE_4, MayaCamera::StandardView::Right},
    {SDL_SCANCODE_5, MayaCamera::StandardView::Top},
    {SDL_SCANCODE_6, MayaCamera::StandardView::Bottom},
    {SDL_SCANCODE_0, MayaCamera::StandardView::Persp},
  }};

  for(const auto &[scancode, view] : views) {
    if(keyboard.keyPressed(scancode)) {
      g_cam2.pushUndo();
      g_cam2.setView(view);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// rotate the 3rd person camera
///////////////////////////////////////////////////////////////////////////////
void RotateCamera(int x, int y) {
  const glm::vec2 delta = {static_cast<float>(x - g_mousePos.x), static_cast<float>(y - g_mousePos.y)};
  g_mousePos = {x, y};

  // ANGLE_SCALE is degrees per pixel, tumble() already applies half of that
  g_cam1.tumble(delta * (ANGLE_SCALE * 2.0F));

  // the observer stays the right way up, unlike the camera it is watching
  g_cam1.setRotation(g_cam1.getYaw(), glm::clamp(g_cam1.getPitch(), -89.0F, 89.0F));
}

///////////////////////////////////////////////////////////////////////////////
// move the 3rd person camera forward/backward
///////////////////////////////////////////////////////////////////////////////
void ZoomCameraDelta(float delta) {
  const float distance = glm::clamp(g_cam1.getDistance() - (delta * ZOOM_SCALE), MIN_DIST, MAX_DIST);
  g_cam1.setDistance(distance);
}

void SetMVP(const glm::mat4 &mvp) {
  glBindBuffer(GL_UNIFORM_BUFFER, g_UBO);
  glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING_POINT, g_UBO);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(mvp));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

namespace {
void drawFlat(const glm::mat4 &mvp, GLenum primitive, const FlatVertex *pVertices, GLsizei count) {
  glUseProgram(g_flatProgram);
  SetMVP(mvp);

  glNamedBufferSubData(g_dynamicVBO, 0, static_cast<GLsizeiptr>(static_cast<size_t>(count) * sizeof(FlatVertex)), pVertices);

  glBindVertexArray(g_dynamicVAO);
  glDrawArrays(primitive, 0, count);
}

void drawObj(const ModelOBJ &model, GLuint vao, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &modelMatrix, const Material &material) {
  glUseProgram(g_litProgram);

  SetMVP(projection * view * modelMatrix);

  const glm::mat4 modelView = view * modelMatrix;
  glUniformMatrix4fv(g_uLitModelView, 1, GL_FALSE, glm::value_ptr(modelView));

  const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelView)));
  glUniformMatrix3fv(g_uLitNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));

  glUniform4fv(g_uLitAmbient, 1, glm::value_ptr(material.ambient));
  glUniform4fv(g_uLitDiffuse, 1, glm::value_ptr(material.diffuse));
  glUniform4fv(g_uLitSpecular, 1, glm::value_ptr(material.specular));
  glUniform1f(g_uLitShininess, material.shininess);

  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(model.getNumberOfIndices()), GL_UNSIGNED_INT, nullptr);
}
}  // namespace

///////////////////////////////////////////////////////////////////////////////
// draw a grid on the XZ-plane
///////////////////////////////////////////////////////////////////////////////
void RenderGrid(const glm::mat4 &mvp) {
  glUseProgram(g_flatProgram);
  SetMVP(mvp);

  glBindVertexArray(g_gridVAO);
  glDrawArrays(GL_LINES, 0, g_gridVertexCount);
}

///////////////////////////////////////////////////////////////////////////////
// draw a line from the camera to its focal point
///////////////////////////////////////////////////////////////////////////////
void RenderFocalLine(const glm::mat4 &mvp) {
  const std::array<FlatVertex, 2> line = {
    FlatVertex{g_cameraPosition, {1.0F, 1.0F, 0.2F, 0.7F}},
    FlatVertex{g_cameraTarget, {1.0F, 1.0F, 0.2F, 0.7F}},
  };

  glDepthFunc(GL_ALWAYS);  // to avoid visual artifacts with grid lines
  drawFlat(mvp, GL_LINES, line.data(), static_cast<GLsizei>(line.size()));
  glDepthFunc(GL_LEQUAL);
}

void RenderFocalPoint(const glm::mat4 &mvp) {
  const std::array<FlatVertex, 1> point = {FlatVertex{g_cameraTarget, {1.0F, 1.0F, 0.2F, 0.7F}}};

  glDepthFunc(GL_ALWAYS);
  glPointSize(5.0F);
  drawFlat(mvp, GL_POINTS, point.data(), static_cast<GLsizei>(point.size()));
  glPointSize(1.0F);
  glDepthFunc(GL_LEQUAL);
}

///////////////////////////////////////////////////////////////////////////////
// draw the field of view cone
//
// NOTE: the original drew this lit and two-sided, once with the front faces
// culled and once with the back ones. The cone is a flat grey fading out to
// nothing, so here it is drawn unlit in a single pass with culling off, which
// lands in the same place.
///////////////////////////////////////////////////////////////////////////////
void RenderFov(const glm::mat4 &mvp) {
  constexpr glm::vec4 nearColor = {0.5F, 0.5F, 0.5F, 0.5F};
  constexpr glm::vec4 farColor = {0.5F, 0.5F, 0.5F, 0.0F};

  const std::array<FlatVertex, 12> faces = {
    // top
    FlatVertex{g_fovVertices[0], nearColor},
    FlatVertex{g_fovVertices[2], farColor},
    FlatVertex{g_fovVertices[1], farColor},
    // bottom
    FlatVertex{g_fovVertices[0], nearColor},
    FlatVertex{g_fovVertices[3], farColor},
    FlatVertex{g_fovVertices[4], farColor},
    // left
    FlatVertex{g_fovVertices[0], nearColor},
    FlatVertex{g_fovVertices[1], farColor},
    FlatVertex{g_fovVertices[3], farColor},
    // right
    FlatVertex{g_fovVertices[0], nearColor},
    FlatVertex{g_fovVertices[4], farColor},
    FlatVertex{g_fovVertices[2], farColor},
  };

  glDisable(GL_CULL_FACE);
  drawFlat(mvp, GL_TRIANGLES, faces.data(), static_cast<GLsizei>(faces.size()));
  glEnable(GL_CULL_FACE);

  constexpr glm::vec4 edgeColor = {0.5F, 0.5F, 0.5F, 0.8F};
  const std::array<FlatVertex, 8> edges = {
    FlatVertex{g_fovVertices[0], edgeColor},
    FlatVertex{g_fovVertices[1], farColor},
    FlatVertex{g_fovVertices[0], edgeColor},
    FlatVertex{g_fovVertices[2], farColor},
    FlatVertex{g_fovVertices[0], edgeColor},
    FlatVertex{g_fovVertices[3], farColor},
    FlatVertex{g_fovVertices[0], edgeColor},
    FlatVertex{g_fovVertices[4], farColor},
  };

  glDisable(GL_DEPTH_TEST);
  drawFlat(mvp, GL_LINES, edges.data(), static_cast<GLsizei>(edges.size()));
  glEnable(GL_DEPTH_TEST);
}

void RenderModel(const glm::mat4 &projection, const glm::mat4 &view) {
  if(!g_objLoaded)
    return;

  drawObj(g_objModel, g_modelVAO, projection, view, glm::mat4(1.0F), DEFAULT_MATERIAL);
}

///////////////////////////////////////////////////////////////////////////////
// draw the camera model, oriented so it points at its focal point
///////////////////////////////////////////////////////////////////////////////
void RenderCameraModel(const glm::mat4 &projection, const glm::mat4 &view) {
  // matModel.translate(cameraPosition) then matModel.lookAt(cameraTarget, up)
  // in the original: the three axes go into the columns and the position into
  // the translation.
  const glm::vec3 forward = glm::normalize(g_cameraTarget - g_cameraPosition);
  const glm::vec3 left = glm::normalize(glm::cross(g_cam2.getUpAxis(), forward));
  const glm::vec3 up = glm::normalize(glm::cross(forward, left));

  glm::mat4 modelMatrix = glm::mat4(1.0F);
  modelMatrix[0] = glm::vec4(left, 0.0F);
  modelMatrix[1] = glm::vec4(up, 0.0F);
  modelMatrix[2] = glm::vec4(forward, 0.0F);
  modelMatrix[3] = glm::vec4(g_cameraPosition, 1.0F);

  if(g_objLoaded)
    drawObj(g_objCam, g_camVAO, projection, view, modelMatrix, CAMERA_MATERIAL);

  if(g_fovEnabled) {
    if(g_cam2.isOrthographic())
      RenderOrthoVolume(projection * view * modelMatrix);
    else
      RenderFov(projection * view * modelMatrix);
  }
}

///////////////////////////////////////////////////////////////////////////////
// the orthographic camera's view volume: a box, not a cone, so the FOV cone is
// replaced by the edges of a prism the width of the ortho view
///////////////////////////////////////////////////////////////////////////////
void RenderOrthoVolume(const glm::mat4 &mvp) {
  constexpr glm::vec4 edgeColor = {0.5F, 0.5F, 0.5F, 0.8F};

  const float h = g_cam2.getOrthoHalfHeight();
  const std::array<glm::vec3, 4> corners = {glm::vec3{h, h, 0.0F}, {-h, h, 0.0F}, {-h, -h, 0.0F}, {h, -h, 0.0F}};

  std::array<FlatVertex, 24> edges{};
  size_t i = 0;
  for(size_t corner = 0; corner < corners.size(); ++corner) {
    const glm::vec3 &a = corners[corner];
    const glm::vec3 &b = corners[(corner + 1) % corners.size()];

    edges[i++] = {a, edgeColor};                                    // near face
    edges[i++] = {b, edgeColor};                                    //
    edges[i++] = {a + glm::vec3{0.0F, 0.0F, FOV_DIST}, edgeColor};  // far face
    edges[i++] = {b + glm::vec3{0.0F, 0.0F, FOV_DIST}, edgeColor};  //
    edges[i++] = {a, edgeColor};                                    // and the side joining them
    edges[i++] = {a + glm::vec3{0.0F, 0.0F, FOV_DIST}, edgeColor};  //
  }

  glDisable(GL_DEPTH_TEST);
  drawFlat(mvp, GL_LINES, edges.data(), static_cast<GLsizei>(edges.size()));
  glEnable(GL_DEPTH_TEST);
}

void RenderThirdPersonView(const glm::ivec4 &viewport) {
  glViewport(viewport.x, viewport.y, viewport.z, viewport.w);
  glScissor(viewport.x, viewport.y, viewport.z, viewport.w);

  const float aspect = static_cast<float>(viewport.z) / static_cast<float>(viewport.w);
  const glm::mat4 projection = g_cam1.getProjectionMatrix(aspect);
  const glm::mat4 view = g_cam1.getViewMatrix();
  const glm::mat4 viewProjection = projection * view;

  if(g_gridEnabled)
    RenderGrid(viewProjection);

  RenderFocalLine(viewProjection);
  RenderFocalPoint(viewProjection);

  RenderModel(projection, view);
  RenderCameraModel(projection, view);
}

void RenderPointOfView(const glm::ivec4 &viewport) {
  glViewport(viewport.x, viewport.y, viewport.z, viewport.w);
  glScissor(viewport.x, viewport.y, viewport.z, viewport.w);

  const float aspect = static_cast<float>(viewport.z) / static_cast<float>(viewport.w);
  const glm::mat4 projection = g_cam2.getProjectionMatrix(aspect);
  const glm::mat4 view = g_cameraMatrix;
  const glm::mat4 viewProjection = projection * view;

  if(g_gridEnabled)
    RenderGrid(viewProjection);

  RenderFocalPoint(viewProjection);
  RenderModel(projection, view);
}

void RenderFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(g_pWindow);
  ImGui::NewFrame();

  { RenderControls(); }
  ImGui::Render();

  // The two GL views split whatever the control panel leaves behind.
  const int panelWidth = std::min(static_cast<int>(PANEL_WIDTH), g_windowResolution.x / 2);
  const int glWidth = g_windowResolution.x - panelWidth;
  const int viewWidth = std::max(glWidth / 2, 1);
  const int height = std::max(g_windowResolution.y, 1);

  g_view1Rect = {0, 0, viewWidth, height};
  g_view2Rect = {viewWidth, 0, std::max(glWidth - viewWidth, 1), height};

  glViewport(0, 0, g_windowResolution.x, g_windowResolution.y);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_SCISSOR_TEST);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  RenderThirdPersonView(g_view1Rect);
  RenderPointOfView(g_view2Rect);

  glDisable(GL_SCISSOR_TEST);
  glViewport(0, 0, g_windowResolution.x, g_windowResolution.y);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

///////////////////////////////////////////////////////////////////////////////
// the control panel, standing in for the original's modeless dialog
///////////////////////////////////////////////////////////////////////////////
void RenderControls() {
  const int panelWidth = std::min(static_cast<int>(PANEL_WIDTH), g_windowResolution.x / 2);
  const auto panelWidthF = static_cast<float>(panelWidth);
  const auto windowHeightF = static_cast<float>(g_windowResolution.y);
  const auto glWidthF = static_cast<float>(g_windowResolution.x - panelWidth);

  // the two viewport captions the original drew with its bitmap font
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(glWidthF, windowHeightF));
  ImGui::Begin("Captions",
               nullptr,
               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
  ImGui::TextColored(ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "3rd Person View");
  ImGui::SameLine(glWidthF * 0.5F);
  ImGui::TextColored(ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "Point of View");
  if(!g_objLoaded)
    ImGui::TextColored(ImVec4(1.0F, 0.4F, 0.4F, 1.0F), "%s", g_objError.c_str());
  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(glWidthF, 0));
  ImGui::SetNextWindowSize(ImVec2(panelWidthF, windowHeightF));
  ImGui::Begin("Maya Camera", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

  ImGui::Text("Rotation (degree)");
  float yaw = g_cam2.getYaw();
  float pitch = g_cam2.getPitch();
  if(ImGui::SliderFloat("Yaw", &yaw, -180.0F, 180.0F, "%.0f") || ImGui::SliderFloat("Pitch", &pitch, -180.0F, 180.0F, "%.0f"))
    g_cam2.setRotation(yaw, pitch);

  float distance = g_cam2.getDistance();
  if(ImGui::SliderFloat("Distance", &distance, 0.1F, 50.0F, "%.2f"))
    g_cam2.setDistance(distance);

  ImGui::Separator();
  ImGui::Text("Center of interest");
  glm::vec3 pivot = g_cam2.getPivot();
  bool pivotChanged = false;
  pivotChanged |= ImGui::SliderFloat("Pivot X", &pivot.x, -10.0F, 10.0F, "%.1f");
  pivotChanged |= ImGui::SliderFloat("Pivot Y", &pivot.y, -10.0F, 10.0F, "%.1f");
  pivotChanged |= ImGui::SliderFloat("Pivot Z", &pivot.z, -10.0F, 10.0F, "%.1f");
  if(pivotChanged)
    g_cam2.setPivot(pivot);

  ImGui::Text("Tumble pivot");
  constexpr std::array<const char *, 3> pivotModes = {"Center of interest", "Origin", "Selection"};
  int pivotMode = static_cast<int>(g_cam2.getTumblePivot());
  if(ImGui::Combo("##tumblePivot", &pivotMode, pivotModes.data(), static_cast<int>(pivotModes.size())))
    g_cam2.setTumblePivot(static_cast<MayaCamera::TumblePivot>(pivotMode));

  ImGui::Separator();
  ImGui::Text("Views");
  constexpr std::array<std::pair<const char *, MayaCamera::StandardView>, 7> views = {{
    {"Persp", MayaCamera::StandardView::Persp},
    {"Front", MayaCamera::StandardView::Front},
    {"Back", MayaCamera::StandardView::Back},
    {"Left", MayaCamera::StandardView::Left},
    {"Right", MayaCamera::StandardView::Right},
    {"Top", MayaCamera::StandardView::Top},
    {"Bottom", MayaCamera::StandardView::Bottom},
  }};
  for(size_t i = 0; i < views.size(); ++i) {
    if(i % 4 != 0)
      ImGui::SameLine();
    if(ImGui::Button(views[i].first)) {
      g_cam2.pushUndo();
      g_cam2.setView(views[i].second);
    }
  }

  bool orthographic = g_cam2.isOrthographic();
  if(ImGui::Checkbox("Orthographic", &orthographic))
    g_cam2.setOrthographic(orthographic);

  ImGui::Separator();
  if(ImGui::Button("Frame model (F)"))
    FrameModel();
  ImGui::SameLine();
  if(ImGui::Button("Frame scene (A)"))
    FrameScene();

  // ImGui 1.81 has no BeginDisabled(); undo()/redo() are no-ops when the stack
  // is empty, so the buttons just report what is left instead.
  if(ImGui::Button("Undo view ([)"))
    g_cam2.undo();
  ImGui::SameLine();
  if(ImGui::Button("Redo view (])"))
    g_cam2.redo();
  ImGui::SameLine();
  ImGui::Text("%zu / %zu", g_cam2.undoDepth(), g_cam2.redoDepth());

  ImGui::Separator();
  ImGui::Checkbox("Grid", &g_gridEnabled);
  ImGui::SameLine();
  ImGui::Checkbox("FOV", &g_fovEnabled);

  // The FOV drives the Point of View projection, the cone drawn in the 3rd
  // person view, and how tight F and A frame, so it stays live even when the
  // cone is hidden.
  if(ImGui::SliderFloat("FOV (degree)", &g_fov, FOV_MIN, FOV_MAX, "%.0f")) {
    g_cam2.setFieldOfView(g_fov);
    ComputeFovVertices(g_fov);
  }

  if(ImGui::Button("Reset"))
    ResetCamera();

  ImGui::Separator();
  ImGui::Text("Eye  %7.3f %7.3f %7.3f",
              static_cast<double>(g_cameraPosition.x),
              static_cast<double>(g_cameraPosition.y),
              static_cast<double>(g_cameraPosition.z));

  ImGui::Text("View matrix");
  for(int row = 0; row < 4; ++row) {
    ImGui::Text("%7.3f %7.3f %7.3f %7.3f",
                static_cast<double>(g_cameraMatrix[0][row]),
                static_cast<double>(g_cameraMatrix[1][row]),
                static_cast<double>(g_cameraMatrix[2][row]),
                static_cast<double>(g_cameraMatrix[3][row]));
  }

  ImGui::Separator();
  ImGui::Text("FPS: %d", g_framesPerSecond);
  ImGui::Text("Vertical sync: %s", g_enableVerticalSync ? "enabled" : "disabled");
  ImGui::TextWrapped("Point of View: ALT + LEFT to tumble, ALT + MIDDLE to track, ALT + RIGHT or the wheel to dolly. "
                     "F frames the model, A the scene, [ and ] undo and redo the view, 1-6 pick an orthographic view "
                     "and 0 goes back to perspective.\n"
                     "3rd Person View: drag LEFT to tumble, RIGHT or the wheel to zoom.\n"
                     "R resets, V toggles vertical sync, ESC exits.");

  ImGui::End();
}

void ToggleFullScreen() {
  // TODO(Hussein): Implement me
}

void UpdateFrame(float elapsedTimeSec) {
  UpdateFrameRate(elapsedTimeSec);

  Keyboard::instance().update();

  ProcessUserInput();

  SyncCameraState();
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

namespace {
void setupFlatFormat(GLuint vbo) {
  constexpr auto PositionID = 0;
  constexpr auto ColorID = 1;

  glBindVertexBuffer(0, vbo, 0, sizeof(FlatVertex));

  glEnableVertexAttribArray(PositionID);
  glVertexAttribFormat(PositionID, 3, GL_FLOAT, GL_FALSE, offsetof(FlatVertex, position));
  glVertexAttribBinding(PositionID, 0);

  glEnableVertexAttribArray(ColorID);
  glVertexAttribFormat(ColorID, 4, GL_FLOAT, GL_FALSE, offsetof(FlatVertex, color));
  glVertexAttribBinding(ColorID, 0);
}
}  // namespace

void createGridBuffers() {
  std::vector<FlatVertex> vertices;

  constexpr glm::vec4 lineColor = {0.5F, 0.5F, 0.5F, 0.5F};

  for(float i = GRID_STEP; i <= GRID_SIZE; i += GRID_STEP) {
    vertices.push_back({{-GRID_SIZE, 0.0F, i}, lineColor});  // lines parallel to X-axis
    vertices.push_back({{GRID_SIZE, 0.0F, i}, lineColor});
    vertices.push_back({{-GRID_SIZE, 0.0F, -i}, lineColor});
    vertices.push_back({{GRID_SIZE, 0.0F, -i}, lineColor});

    vertices.push_back({{i, 0.0F, -GRID_SIZE}, lineColor});  // lines parallel to Z-axis
    vertices.push_back({{i, 0.0F, GRID_SIZE}, lineColor});
    vertices.push_back({{-i, 0.0F, -GRID_SIZE}, lineColor});
    vertices.push_back({{-i, 0.0F, GRID_SIZE}, lineColor});
  }

  // x-axis
  constexpr glm::vec4 xAxisColor = {1.0F, 0.0F, 0.0F, 1.0F};
  vertices.push_back({{-GRID_SIZE, 0.0F, 0.0F}, xAxisColor});
  vertices.push_back({{GRID_SIZE, 0.0F, 0.0F}, xAxisColor});

  // z-axis
  constexpr glm::vec4 zAxisColor = {0.0F, 0.0F, 1.0F, 0.5F};
  vertices.push_back({{0.0F, 0.0F, -GRID_SIZE}, zAxisColor});
  vertices.push_back({{0.0F, 0.0F, GRID_SIZE}, zAxisColor});

  g_gridVertexCount = static_cast<GLsizei>(vertices.size());

  glGenVertexArrays(1, &g_gridVAO);
  glBindVertexArray(g_gridVAO);

  glCreateBuffers(1, &g_gridVBO);
  glNamedBufferStorage(g_gridVBO, static_cast<GLsizeiptr>(vertices.size() * sizeof(FlatVertex)), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

  setupFlatFormat(g_gridVBO);
}

///////////////////////////////////////////////////////////////////////////////
// one scratch buffer, refilled per draw, for the focal line/point and the FOV
///////////////////////////////////////////////////////////////////////////////
void createDynamicBuffers() {
  // the largest single draw is the 12 edges of the orthographic view volume
  constexpr size_t capacity = 32;

  glGenVertexArrays(1, &g_dynamicVAO);
  glBindVertexArray(g_dynamicVAO);

  glCreateBuffers(1, &g_dynamicVBO);
  glNamedBufferStorage(g_dynamicVBO, static_cast<GLsizeiptr>(capacity * sizeof(FlatVertex)), nullptr, GL_DYNAMIC_STORAGE_BIT);

  setupFlatFormat(g_dynamicVBO);
}

void uploadObj(const ModelOBJ &model, GLuint &vao, GLuint &vbo, GLuint &ebo) {
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  const auto vertexSize = static_cast<size_t>(model.getVertexSize());
  const auto vertexCount = static_cast<size_t>(model.getNumberOfVertices());
  const auto indexCount = static_cast<size_t>(model.getNumberOfIndices());

  glCreateBuffers(1, &vbo);
  glNamedBufferStorage(vbo, static_cast<GLsizeiptr>(vertexCount * vertexSize), model.getVertexBuffer(), GL_DYNAMIC_STORAGE_BIT);

  glCreateBuffers(1, &ebo);
  glNamedBufferStorage(ebo, static_cast<GLsizeiptr>(indexCount * sizeof(int)), model.getIndexBuffer(), GL_DYNAMIC_STORAGE_BIT);

  constexpr auto PositionID = 0;
  constexpr auto NormalID = 1;

  glBindVertexBuffer(0, vbo, 0, static_cast<GLsizei>(vertexSize));

  // ModelOBJ::Vertex is {position[3], texCoord[2], normal[3]}
  glEnableVertexAttribArray(PositionID);
  glVertexAttribFormat(PositionID, 3, GL_FLOAT, GL_FALSE, offsetof(ModelOBJ::Vertex, position));
  glVertexAttribBinding(PositionID, 0);

  glEnableVertexAttribArray(NormalID);
  glVertexAttribFormat(NormalID, 3, GL_FLOAT, GL_FALSE, offsetof(ModelOBJ::Vertex, normal));
  glVertexAttribBinding(NormalID, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
}

void createFlatProgram() {
  constexpr std::string_view VertexShader = R"(
  #version 460 core

  layout(location=0) in vec3 aPosition;
  layout(location=1) in vec4 aColor;

  layout(std140, binding=0) uniform Matrices
  {
      mat4 uMVP;
  };

  out Interpolants {
    vec4 wColor;
  } OUT;

  void main() {
    OUT.wColor = aColor;
    gl_Position = uMVP * vec4(aPosition, 1.0);
  }
  )";
  constexpr std::string_view FragmentShader = R"(
  #version 460 core

  in Interpolants {
    vec4 wColor;
  } IN;

  layout(location=0) out vec4 out_Color;

  void main() {
    out_Color = IN.wColor;
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

  g_flatProgram = static_cast<GLuint>(program);
}

void createLitProgram() {
  constexpr std::string_view VertexShader = R"(
  #version 460 core

  layout(location=0) in vec3 aPosition;
  layout(location=1) in vec3 aNormal;

  layout(std140, binding=0) uniform Matrices
  {
      mat4 uMVP;
  };

  uniform mat4 uModelView;
  uniform mat3 uNormalMatrix;

  out Interpolants {
    vec3 esVertex;
    vec3 esNormal;
  } OUT;

  void main() {
    OUT.esVertex = vec3(uModelView * vec4(aPosition, 1.0));
    OUT.esNormal = uNormalMatrix * aNormal;
    gl_Position = uMVP * vec4(aPosition, 1.0);
  }
  )";
  constexpr std::string_view FragmentShader = R"(
  #version 460 core

  in Interpolants {
    vec3 esVertex;
    vec3 esNormal;
  } IN;

  uniform vec4 uAmbient;
  uniform vec4 uDiffuse;
  uniform vec4 uSpecular;
  uniform float uShininess;

  layout(location=0) out vec4 out_Color;

  // The single light of the original: a directional light along +z in eye
  // space, no ambient of its own beyond 0.2, a 0.8 grey diffuse and a white
  // specular.
  const vec4 LIGHT_AMBIENT = vec4(0.2, 0.2, 0.2, 1.0);
  const vec4 LIGHT_DIFFUSE = vec4(0.8, 0.8, 0.8, 1.0);
  const vec4 LIGHT_SPECULAR = vec4(1.0, 1.0, 1.0, 1.0);
  const vec3 LIGHT_DIRECTION = vec3(0.0, 0.0, 1.0);

  void main() {
    vec3 normal = normalize(IN.esNormal);
    vec3 light = normalize(LIGHT_DIRECTION);
    vec3 view = normalize(-IN.esVertex);
    vec3 halfv = normalize(light + view);

    vec4 color = uAmbient * LIGHT_AMBIENT;

    float dotNL = max(dot(normal, light), 0.0);
    color += uDiffuse * LIGHT_DIFFUSE * dotNL;

    float dotNH = max(dot(normal, halfv), 0.0);
    color += uSpecular * LIGHT_SPECULAR * pow(dotNH, uShininess);

    out_Color = vec4(color.rgb, uDiffuse.a);
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

  g_litProgram = static_cast<GLuint>(program);

  g_uLitModelView = glGetUniformLocation(g_litProgram, "uModelView");
  g_uLitNormalMatrix = glGetUniformLocation(g_litProgram, "uNormalMatrix");
  g_uLitAmbient = glGetUniformLocation(g_litProgram, "uAmbient");
  g_uLitDiffuse = glGetUniformLocation(g_litProgram, "uDiffuse");
  g_uLitSpecular = glGetUniformLocation(g_litProgram, "uSpecular");
  g_uLitShininess = glGetUniformLocation(g_litProgram, "uShininess");
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
  if(g_gridVAO) {
    glDeleteVertexArrays(1, &g_gridVAO);
    g_gridVAO = 0;
  }
  if(g_gridVBO) {
    glDeleteBuffers(1, &g_gridVBO);
    g_gridVBO = 0;
  }

  if(g_dynamicVAO) {
    glDeleteVertexArrays(1, &g_dynamicVAO);
    g_dynamicVAO = 0;
  }
  if(g_dynamicVBO) {
    glDeleteBuffers(1, &g_dynamicVBO);
    g_dynamicVBO = 0;
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

  if(g_camVAO) {
    glDeleteVertexArrays(1, &g_camVAO);
    g_camVAO = 0;
  }
  if(g_camVBO) {
    glDeleteBuffers(1, &g_camVBO);
    g_camVBO = 0;
  }
  if(g_camEBO) {
    glDeleteBuffers(1, &g_camEBO);
    g_camEBO = 0;
  }

  if(g_flatProgram) {
    glDeleteProgram(g_flatProgram);
    g_flatProgram = 0;
  }
  if(g_litProgram) {
    glDeleteProgram(g_litProgram);
    g_litProgram = 0;
  }

  if(g_UBO) {
    glDeleteBuffers(1, &g_UBO);
    g_UBO = 0;
  }
}
