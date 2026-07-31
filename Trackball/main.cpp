///////////////////////////////////////////////////////////////////////////////
// main.cpp
// ========
// 3D drawing for quaternion
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2011-12-09
// UPDATED: 2016-04-05
///////////////////////////////////////////////////////////////////////////////
//
// This demo visualises how a trackball maps the mouse cursor onto a sphere and
// turns the movement between two of those points into a rotation quaternion.
//
// Drag with the left mouse button to rotate, drag with the right mouse button
// to dolly the camera in and out. SPACE switches the trackball between its two
// mapping modes: ARC maps the cursor's distance from the screen centre to an
// arc length on the sphere (and so reaches the back hemisphere), while PROJECT
// projects the cursor straight onto the sphere, falling back to a hyperbolic
// sheet past r^2/2 (and so is limited to the front hemisphere).
//
// SDL2 + OpenGL 4.6 port of the original GLUT demo.
//-----------------------------------------------------------------------------
// SDL2
#include <SDL2/SDL.h>
// glm
#include <glm/gtc/matrix_transform.hpp>
//
#include "input.hpp"
#include "model_obj.hpp"
#include "shaders.hpp"
#include "trackball.hpp"

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------

namespace {
constexpr auto APP_TITLE = "OpenGL Trackball Demo";

constexpr float PI = 3.141592F;
constexpr float RAD2DEG = 180.0F / PI;

constexpr float RADIUS_SCALE = 0.5F;
constexpr int CIRCLE_STEPS = 100;
constexpr int PATH_COUNT = 30;

constexpr int SPHERE_STACKS = 32;
constexpr int SPHERE_SLICES = 32;
constexpr int TORUS_RINGS = 32;
constexpr int TORUS_SIDES = 16;
constexpr float TORUS_INNER_RADIUS = 0.2F;

constexpr float CAMERA_FOVY = 60.0F;
constexpr float CAMERA_ZNEAR = 1.0F;
constexpr float CAMERA_ZFAR = 10000.0F;

constexpr uint32_t MATRICES_BINDING_POINT = 0;

constexpr auto OBJECT_FILE = "data/debugger_small_5k.obj";

// The original demo drew the mouse markers with gluSphere() at a radius of
// 5% of the trackball, and the rotating object with glutWireTeapot() at 60%.
constexpr float MARKER_SCALE = 0.05F;
constexpr float OBJECT_SCALE = 0.6F;

struct MeshVertex {
  glm::vec3 position;
  glm::vec3 normal;
};

struct FlatVertex {
  glm::vec3 position;
  glm::vec4 color;
};
}  // namespace

//-----------------------------------------------------------------------------
// Globals.
//-----------------------------------------------------------------------------

int g_framesPerSecond;
glm::ivec2 g_windowResolution;
bool g_hasFocus;
bool g_enableVerticalSync;
bool g_displayHelp;
SDL_Window *g_pWindow = nullptr;
SDL_GLContext g_glcontext = nullptr;

// The demo's state, straight out of the original's initSharedMem().
Trackball g_trackball;
glm::quat g_quat = {1.0F, 0.0F, 0.0F, 0.0F};
glm::quat g_prevQuat = {1.0F, 0.0F, 0.0F, 0.0F};
glm::vec3 g_sphereVector;
float g_sphereRadius;
float g_cameraDistance;
int g_drawMode;
bool g_mouseLeftDown;
bool g_mouseRightDown;
glm::ivec2 g_mousePos;
glm::ivec2 g_prevMousePos;
std::vector<glm::vec3> g_circlePoints;
std::vector<glm::vec3> g_pathPoints;

GLuint g_UBO = 0;

GLuint g_flatProgram = 0;

GLuint g_litProgram = 0;
GLint g_uLitModelView = -1;
GLint g_uLitNormalMatrix = -1;
GLint g_uLitColor = -1;
GLint g_uLitLightPos = -1;

GLuint g_sphereVAO = 0;
GLuint g_sphereVBO = 0;
GLuint g_sphereEBO = 0;
GLsizei g_sphereIndexCount = 0;

GLuint g_objectVAO = 0;
GLuint g_objectVBO = 0;
GLuint g_objectEBO = 0;
GLsizei g_objectIndexCount = 0;
GLenum g_objectIndexType = GL_UNSIGNED_SHORT;

ModelOBJ g_objectModel;
bool g_objectModelLoaded = false;

GLuint g_axisLinesVAO = 0;
GLuint g_axisLinesVBO = 0;

GLuint g_dynamicVAO = 0;
GLuint g_dynamicVBO = 0;

//-----------------------------------------------------------------------------
// Functions Prototypes.
//-----------------------------------------------------------------------------

void Cleanup();
void CleanupApp();
std::vector<glm::vec3> BuildCircle(float radius, int steps);
void GenerateMousePath();
float GetElapsedTimeInSeconds();
bool Init();
void InitApp();
void InitGL();
void InitImgui();
void Log(const char *pszMessage);
void ProcessUserInput();
void RenderAxis(const glm::mat4 &mvp, float size);
void RenderFrame();
void RenderObject(const glm::mat4 &projection, const glm::mat4 &view);
void RenderOverlay2D();
void RenderPointers(const glm::mat4 &projection);
void RenderText();
void ResetTrackball();
void SetDrawMode(int mode);
void SetMVP(const glm::mat4 &mvp);
void ToggleFullScreen();
void UpdateFrame(float elapsedTimeSec);
void UpdateFrameRate(float elapsedTimeSec);
void createAxisBuffers();
void createDynamicBuffers();
void createObjectBuffers();
void createSphereBuffers();
void createUniformBuffers();
void createFlatProgram();
void createLitProgram();

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

  // The trackball is driven by the raw cursor position, so the mouse is left
  // alone here: no grabbing, no recentering, cursor visible.
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

        if(SDL_QUIT == event.type) {
          bRunning = false;
        }
        if(SDL_KEYDOWN == event.type) {
          if(SDL_SCANCODE_ESCAPE == event.key.keysym.scancode) {
            bRunning = false;
          }
        }
        if(SDL_MOUSEBUTTONDOWN == event.type) {
          g_mousePos = {event.button.x, event.button.y};
          if(SDL_BUTTON_LEFT == event.button.button) {
            g_mouseLeftDown = true;
            // remember mouse coords and quaternion before rotation
            g_prevMousePos = g_mousePos;
            g_prevQuat = g_quat;
          } else if(SDL_BUTTON_RIGHT == event.button.button) {
            g_mouseRightDown = true;
          }
        }
        if(SDL_MOUSEBUTTONUP == event.type) {
          g_mousePos = {event.button.x, event.button.y};
          if(SDL_BUTTON_LEFT == event.button.button) {
            g_mouseLeftDown = false;
            g_pathPoints.clear();  // clear mouse path
          } else if(SDL_BUTTON_RIGHT == event.button.button) {
            g_mouseRightDown = false;
          }
        }
        if(SDL_MOUSEMOTION == event.type) {
          const int x = event.motion.x;
          const int y = event.motion.y;

          if(g_mouseLeftDown) {
            const glm::vec3 v1 = g_trackball.getUnitVector(g_prevMousePos.x, g_prevMousePos.y);
            const glm::vec3 v2 = g_trackball.getUnitVector(x, y);

            // NOTE: mathlib composed quaternions left to right, so the
            // original's "delta * prevQuat" keeps its order under glm only
            // because both use the standard Hamilton product here.
            const glm::quat delta = Trackball::getQuaternion(v1, v2);
            g_quat = delta * g_prevQuat;

            g_mousePos = {x, y};

            GenerateMousePath();  // compute mouse path
          } else if(g_mouseRightDown) {
            g_cameraDistance -= static_cast<float>(y - g_mousePos.y) * (g_sphereRadius * 0.01F);
            g_mousePos.y = y;
          } else {
            // passive motion: the cursor picks the first point on the sphere
            g_mousePos = {x, y};
            g_sphereVector = g_trackball.getVector(g_mousePos.x, g_mousePos.y);
          }
        }
        if(SDL_WINDOWEVENT == event.type) {
          const auto windowEvent = event.window;
          switch(windowEvent.event) {
          case SDL_WINDOWEVENT_RESIZED:
            g_windowResolution = {static_cast<int>(windowEvent.data1), static_cast<int>(windowEvent.data2)};
            ResetTrackball();
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

///////////////////////////////////////////////////////////////////////////////
// make points of a circle
///////////////////////////////////////////////////////////////////////////////
std::vector<glm::vec3> BuildCircle(float radius, int steps) {
  std::vector<glm::vec3> points;
  if(steps < 2)
    return points;

  constexpr float PI2 = PI * 2.0F;
  points.reserve(static_cast<size_t>(steps) + 1);
  for(int i = 0; i <= steps; ++i) {
    const float a = PI2 / static_cast<float>(steps) * static_cast<float>(i);
    points.push_back({radius * std::cos(a), radius * std::sin(a), 0.0F});
  }
  return points;
}

///////////////////////////////////////////////////////////////////////////////
// compute mouse path on a sphere between 2 mouse positions
///////////////////////////////////////////////////////////////////////////////
void GenerateMousePath() {
  const glm::vec3 v1 = g_sphereVector;
  const glm::vec3 v2 = g_trackball.getVector(g_mousePos.x, g_mousePos.y);

  const float lengths = glm::length(v1) * glm::length(v2);
  if(lengths == 0.0F)
    return;

  const float cosine = glm::clamp(glm::dot(v1, v2) / lengths, -1.0F, 1.0F);
  const float angle = std::acos(cosine);
  const float sine = std::sin(angle);

  g_pathPoints.clear();
  for(int i = 0; i <= PATH_COUNT; ++i) {
    const float alpha = static_cast<float>(i) / static_cast<float>(PATH_COUNT);

    // Slerp between the two points on the sphere. When the two vectors are
    // (nearly) colinear the sine goes to zero and the scale factors blow up,
    // so fall back to a plain lerp there.
    glm::vec3 v;
    if(std::abs(sine) < 0.001F) {
      v = v1 + (v2 - v1) * alpha;
    } else {
      const float scale1 = std::sin((1.0F - alpha) * angle) / sine;
      const float scale2 = std::sin(alpha * angle) / sine;
      v = scale1 * v1 + scale2 * v2;
    }

    if(glm::dot(v, v) == 0.0F)
      continue;

    // lift the path a hair off the sphere so it does not z-fight with it
    g_pathPoints.push_back(glm::normalize(v) * (g_sphereRadius + 0.3F));
  }
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
  ResetTrackball();

  createUniformBuffers();
  createSphereBuffers();
  createObjectBuffers();
  createAxisBuffers();
  createDynamicBuffers();
  createFlatProgram();
  createLitProgram();

  SetDrawMode(0);
}

///////////////////////////////////////////////////////////////////////////////
// size the trackball to the window, as the original reshapeCB() did
///////////////////////////////////////////////////////////////////////////////
void ResetTrackball() {
  if(g_windowResolution.x > g_windowResolution.y)
    g_sphereRadius = static_cast<float>(g_windowResolution.y) * RADIUS_SCALE;
  else
    g_sphereRadius = static_cast<float>(g_windowResolution.x) * RADIUS_SCALE;

  g_trackball.set(g_sphereRadius, g_windowResolution.x, g_windowResolution.y);
  g_cameraDistance = g_sphereRadius * 3.0F;
  g_circlePoints = BuildCircle(g_sphereRadius, CIRCLE_STEPS);
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

  InitImgui();
}

void InitImgui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  [[maybe_unused]] ImGuiIO &io = ImGui::GetIO();
  ImGui_ImplSDL2_InitForOpenGL(g_pWindow, g_glcontext);
  ImGui_ImplOpenGL3_Init();
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

  // switch rendering modes (fill -> wire -> point)
  if(keyboard.keyPressed(SDL_SCANCODE_D))
    SetDrawMode((g_drawMode + 1) % 3);

  // reset rotation
  if(keyboard.keyPressed(SDL_SCANCODE_R))
    g_quat = {1.0F, 0.0F, 0.0F, 0.0F};

  // switch the trackball between its two mapping modes
  if(keyboard.keyPressed(SDL_SCANCODE_SPACE)) {
    g_trackball.setMode(g_trackball.getMode() == Trackball::ARC ? Trackball::PROJECT : Trackball::ARC);
  }
}

void SetDrawMode(int mode) {
  g_drawMode = mode;

  if(g_drawMode == 0) {  // fill mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
  } else if(g_drawMode == 1) {  // wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
  } else {  // point mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
  }
}

void SetMVP(const glm::mat4 &mvp) {
  glBindBuffer(GL_UNIFORM_BUFFER, g_UBO);
  glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING_POINT, g_UBO);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(mvp));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

///////////////////////////////////////////////////////////////////////////////
// draw a lit, uniformly coloured mesh
///////////////////////////////////////////////////////////////////////////////
namespace {
void drawLitMesh(const glm::mat4 &projection,
                 const glm::mat4 &view,
                 const glm::mat4 &model,
                 const glm::vec4 &color,
                 GLuint vao,
                 GLsizei indexCount,
                 GLenum indexType = GL_UNSIGNED_SHORT) {
  glUseProgram(g_litProgram);

  SetMVP(projection * view * model);

  const glm::mat4 modelView = view * model;
  glUniformMatrix4fv(g_uLitModelView, 1, GL_FALSE, glm::value_ptr(modelView));

  const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelView)));
  glUniformMatrix3fv(g_uLitNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));

  glUniform4fv(g_uLitColor, 1, glm::value_ptr(color));

  // The original positioned GL_LIGHT0 while the modelview matrix was still the
  // identity, which parks the light in eye space rather than world space.
  const glm::vec3 lightPos = {0.0F, g_sphereRadius * 3.0F, g_sphereRadius * 2.0F};
  glUniform3fv(g_uLitLightPos, 1, glm::value_ptr(lightPos));

  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, indexCount, indexType, nullptr);
}

void drawFlat(const glm::mat4 &mvp, GLenum primitive, const FlatVertex *pVertices, GLsizei count) {
  glUseProgram(g_flatProgram);
  SetMVP(mvp);

  glNamedBufferSubData(g_dynamicVBO, 0, static_cast<GLsizeiptr>(static_cast<size_t>(count) * sizeof(FlatVertex)), pVertices);

  glBindVertexArray(g_dynamicVAO);
  glDrawArrays(primitive, 0, count);
}
}  // namespace

///////////////////////////////////////////////////////////////////////////////
// draw the mouse path, the two vectors on the sphere and the cursor markers
//
// NOTE: the original reset the modelview to the identity here, so none of this
// is affected by the trackball rotation; only the camera dolly applies.
///////////////////////////////////////////////////////////////////////////////
void RenderPointers(const glm::mat4 &projection) {
  const glm::mat4 view = glm::translate(glm::mat4(1.0F), {0.0F, 0.0F, -g_cameraDistance});
  const glm::vec3 vec = g_trackball.getVector(g_mousePos.x, g_mousePos.y);

  // mouse path
  if(g_pathPoints.size() > 1) {
    std::vector<FlatVertex> path;
    path.reserve(g_pathPoints.size());
    for(const glm::vec3 &point : g_pathPoints)
      path.push_back({point, {1.0F, 1.0F, 0.0F, 1.0F}});

    glLineWidth(5.0F);
    drawFlat(projection * view, GL_LINE_STRIP, path.data(), static_cast<GLsizei>(path.size()));
  }

  // the two vectors from the centre of the sphere
  const std::array<FlatVertex, 4> vectors = {
    FlatVertex{{0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 0.0F, 1.0F}},
    FlatVertex{g_sphereVector, {1.0F, 1.0F, 0.0F, 1.0F}},
    FlatVertex{{0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 0.0F, 1.0F}},
    FlatVertex{vec, {1.0F, 1.0F, 0.0F, 1.0F}},
  };
  glLineWidth(2.0F);
  drawFlat(projection * view, GL_LINES, vectors.data(), static_cast<GLsizei>(vectors.size()));
  glLineWidth(1.0F);

  const float markerRadius = g_sphereRadius * MARKER_SCALE;

  // the point on the sphere where the mouse was clicked
  if(g_mouseLeftDown) {
    const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0F), g_sphereVector), glm::vec3(markerRadius));
    drawLitMesh(projection, view, model, {1.0F, 0.0F, 1.0F, 1.0F}, g_sphereVAO, g_sphereIndexCount);
  }

  // the point where the mouse currently is
  {
    const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0F), vec), glm::vec3(markerRadius));
    drawLitMesh(projection, view, model, {0.0F, 1.0F, 1.0F, 1.0F}, g_sphereVAO, g_sphereIndexCount);
  }
}

///////////////////////////////////////////////////////////////////////////////
// draw the local axis of an object
///////////////////////////////////////////////////////////////////////////////
void RenderAxis(const glm::mat4 &mvp, float size) {
  glUseProgram(g_flatProgram);
  SetMVP(glm::scale(mvp, glm::vec3(size)));

  glBindVertexArray(g_axisLinesVAO);

  glLineWidth(3.0F);
  glDrawArrays(GL_LINES, 0, 6);
  glLineWidth(1.0F);

  // draw arrows (actually big square dots)
  glPointSize(5.0F);
  glDrawArrays(GL_POINTS, 1, 1);
  glDrawArrays(GL_POINTS, 3, 1);
  glDrawArrays(GL_POINTS, 5, 1);
  glPointSize(1.0F);
}

///////////////////////////////////////////////////////////////////////////////
// draw the rotating object inside the translucent trackball sphere
//
// NOTE: the original drew glutWireTeapot() here. GLUT is gone and the Newell
// teapot's Bezier patch data never lived in this repository, so this loads
// debugger_small_5k.obj if it's present (drop it in Trackball/data/, same file
// OrbitCamera uses) and falls back to a generated wire torus otherwise: it is
// just as obviously asymmetric under rotation, which is the only job the
// teapot had.
///////////////////////////////////////////////////////////////////////////////
void RenderObject(const glm::mat4 &projection, const glm::mat4 &view) {
  const glm::mat4 objectModel = glm::scale(glm::mat4(1.0F), glm::vec3(g_sphereRadius * OBJECT_SCALE));
  const glm::mat4 sphereModel = glm::scale(glm::mat4(1.0F), glm::vec3(g_sphereRadius));

  // back faces of the object first, so they show through the sphere
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glCullFace(GL_FRONT);
  drawLitMesh(projection, view, objectModel, {1.0F, 1.0F, 1.0F, 1.0F}, g_objectVAO, g_objectIndexCount, g_objectIndexType);

  // then the translucent sphere and the object's front faces
  glCullFace(GL_BACK);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  drawLitMesh(projection, view, sphereModel, {0.7F, 0.7F, 0.7F, 0.6F}, g_sphereVAO, g_sphereIndexCount);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  drawLitMesh(projection, view, objectModel, {0.7F, 0.7F, 0.7F, 0.6F}, g_objectVAO, g_objectIndexCount, g_objectIndexType);

  SetDrawMode(g_drawMode);  // restore the polygon mode the user asked for
}

///////////////////////////////////////////////////////////////////////////////
// draw the 2D trackball circle in screen space
///////////////////////////////////////////////////////////////////////////////
void RenderOverlay2D() {
  if(g_circlePoints.size() < 2)
    return;

  const float halfWidth = static_cast<float>(g_windowResolution.x) * 0.5F;
  const float halfHeight = static_cast<float>(g_windowResolution.y) * 0.5F;
  const glm::mat4 ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -100.0F, 100.0F);

  std::vector<FlatVertex> circle;
  circle.reserve(g_circlePoints.size());
  for(const glm::vec3 &point : g_circlePoints)
    circle.push_back({point, {1.0F, 1.0F, 0.0F, 1.0F}});

  glLineWidth(1.0F);
  drawFlat(ortho, GL_LINE_STRIP, circle.data(), static_cast<GLsizei>(circle.size()));
}

void RenderFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(g_pWindow);
  ImGui::NewFrame();

  { RenderText(); }
  ImGui::Render();

  glViewport(0, 0, g_windowResolution.x, g_windowResolution.y);
  glClearColor(0.0F, 0.0F, 0.0F, 0.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  const float aspect = static_cast<float>(g_windowResolution.x) / static_cast<float>(g_windowResolution.y);
  const glm::mat4 projection = glm::perspective(glm::radians(CAMERA_FOVY), aspect, CAMERA_ZNEAR, CAMERA_ZFAR);

  // The camera transform: the trackball quaternion rotates the scene, then it
  // is pushed away from the eye. mathlib stored its rotation matrix
  // transposed, so the original had to call transpose() before translating;
  // glm::mat4_cast() already hands back the untransposed rotation.
  const glm::mat4 view = glm::translate(glm::mat4(1.0F), {0.0F, 0.0F, -g_cameraDistance}) * glm::mat4_cast(g_quat);

  RenderPointers(projection);
  RenderAxis(projection * view, g_sphereRadius + g_sphereRadius * 0.1F);
  RenderObject(projection, view);
  RenderOverlay2D();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RenderText() {
  std::ostringstream output;

  if(g_displayHelp) {
    output << "Drag the LEFT mouse button to rotate the trackball" << std::endl
           << "Drag the RIGHT mouse button to zoom in/out" << std::endl
           << "Press SPACE to change the trackball mode" << std::endl
           << "Press D to switch the draw mode (fill/wire/point)" << std::endl
           << "Press R to reset the rotation" << std::endl
           << std::endl
           << "Press V to enable/disable vertical sync" << std::endl
           << "Press ALT and ENTER to toggle full screen" << std::endl
           << "Press ESC to exit" << std::endl
           << std::endl
           << "Press H to hide help";
  } else {
    const glm::vec3 v1 = g_sphereVector;                                     // first vector on sphere
    const glm::vec3 v2 = g_trackball.getVector(g_mousePos.x, g_mousePos.y);  // second vector on sphere

    const float lengths = glm::length(v1) * glm::length(v2);
    const float angle = (lengths == 0.0F) ? 0.0F : RAD2DEG * std::acos(glm::clamp(glm::dot(v1, v2) / lengths, -1.0F, 1.0F));

    output << std::fixed << std::setprecision(3);
    output << "Mouse Coords: (" << g_mousePos.x << ", " << g_mousePos.y << ")" << std::endl;

    output << "Point on Sphere: (" << v1.x << ", " << v1.y << ", " << v1.z << ")";
    if(g_mouseLeftDown)
      output << " - (" << v2.x << ", " << v2.y << ", " << v2.z << ")";
    output << std::endl;

    const glm::vec3 n1 = (glm::dot(v1, v1) == 0.0F) ? v1 : glm::normalize(v1);
    const glm::vec3 n2 = (glm::dot(v2, v2) == 0.0F) ? v2 : glm::normalize(v2);
    output << "Normalized: (" << n1.x << ", " << n1.y << ", " << n1.z << ")";
    if(g_mouseLeftDown)
      output << " - (" << n2.x << ", " << n2.y << ", " << n2.z << ")";
    output << std::endl;

    output << "Angle between Points: " << angle << " deg" << std::endl
           << "Mode: " << (g_trackball.getMode() == Trackball::ARC ? "ARC" : "PROJECT") << std::endl
           << "FPS: " << g_framesPerSecond << std::endl
           << "Vertical sync: " << (g_enableVerticalSync ? "enabled" : "disabled") << std::endl
           << std::endl
           << "Press H to display help";
  }

  // NoInputs keeps the overlay from swallowing the drags the trackball needs.
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(g_windowResolution.x), static_cast<float>(g_windowResolution.y)));
  ImGui::Begin("Text",
               nullptr,
               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
  ImGui::TextColored(ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "%s", output.str().c_str());
  ImGui::End();
}

void ToggleFullScreen() {
  // TODO(Hussein): Implement me
}

void UpdateFrame(float elapsedTimeSec) {
  UpdateFrameRate(elapsedTimeSec);

  Keyboard::instance().update();

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

inline size_t uboAligned(size_t size) { return ((size + 255) / 256) * 256; }

void createUniformBuffers() {
  glCreateBuffers(1, &g_UBO);
  glNamedBufferStorage(g_UBO, uboAligned(sizeof(glm::mat4)), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

namespace {
void uploadMesh(GLuint &vao, GLuint &vbo, GLuint &ebo, const std::vector<MeshVertex> &vertices, const std::vector<uint16_t> &elements) {
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glCreateBuffers(1, &vbo);
  glNamedBufferStorage(vbo, static_cast<GLsizeiptr>(vertices.size() * sizeof(MeshVertex)), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

  glCreateBuffers(1, &ebo);
  glNamedBufferStorage(ebo, static_cast<GLsizeiptr>(elements.size() * sizeof(uint16_t)), elements.data(), GL_DYNAMIC_STORAGE_BIT);

  constexpr auto PositionID = 0;
  constexpr auto NormalID = 1;

  glBindVertexBuffer(0, vbo, 0, sizeof(MeshVertex));

  glEnableVertexAttribArray(PositionID);
  glVertexAttribFormat(PositionID, 3, GL_FLOAT, GL_FALSE, offsetof(MeshVertex, position));
  glVertexAttribBinding(PositionID, 0);

  glEnableVertexAttribArray(NormalID);
  glVertexAttribFormat(NormalID, 3, GL_FLOAT, GL_FALSE, offsetof(MeshVertex, normal));
  glVertexAttribBinding(NormalID, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
}

// ModelOBJ::Vertex is {position[3], texCoord[2], normal[3]} and its indices
// are plain int, unlike the uint16_t the generated meshes use.
void uploadObjModel(GLuint &vao, GLuint &vbo, GLuint &ebo, const ModelOBJ &model) {
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

  glEnableVertexAttribArray(PositionID);
  glVertexAttribFormat(PositionID, 3, GL_FLOAT, GL_FALSE, offsetof(ModelOBJ::Vertex, position));
  glVertexAttribBinding(PositionID, 0);

  glEnableVertexAttribArray(NormalID);
  glVertexAttribFormat(NormalID, 3, GL_FLOAT, GL_FALSE, offsetof(ModelOBJ::Vertex, normal));
  glVertexAttribBinding(NormalID, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
}
}  // namespace

///////////////////////////////////////////////////////////////////////////////
// a unit UV sphere, standing in for the gluSphere() of the original
///////////////////////////////////////////////////////////////////////////////
void createSphereBuffers() {
  std::vector<MeshVertex> vertices;
  std::vector<uint16_t> elements;

  vertices.reserve(static_cast<size_t>(SPHERE_STACKS + 1) * static_cast<size_t>(SPHERE_SLICES + 1));
  elements.reserve(static_cast<size_t>(SPHERE_STACKS) * static_cast<size_t>(SPHERE_SLICES) * 6);

  // Like gluSphere the poles sit on the z axis.
  for(int stack = 0; stack <= SPHERE_STACKS; ++stack) {
    const float v = static_cast<float>(stack) / static_cast<float>(SPHERE_STACKS);
    const float phi = v * PI;
    const float cosPhi = std::cos(phi);
    const float sinPhi = std::sin(phi);

    for(int slice = 0; slice <= SPHERE_SLICES; ++slice) {
      const float u = static_cast<float>(slice) / static_cast<float>(SPHERE_SLICES);
      const float theta = u * 2.0F * PI;

      const glm::vec3 normal = {sinPhi * std::cos(theta), sinPhi * std::sin(theta), cosPhi};
      vertices.push_back({normal, normal});
    }
  }

  const auto verticesPerRow = SPHERE_SLICES + 1;

  for(int stack = 0; stack < SPHERE_STACKS; ++stack) {
    for(int slice = 0; slice < SPHERE_SLICES; ++slice) {
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

  g_sphereIndexCount = static_cast<GLsizei>(elements.size());
  uploadMesh(g_sphereVAO, g_sphereVBO, g_sphereEBO, vertices, elements);
}

///////////////////////////////////////////////////////////////////////////////
// a unit torus, the fallback object when OBJECT_FILE is not available
///////////////////////////////////////////////////////////////////////////////
void createTorusBuffers() {
  std::vector<MeshVertex> vertices;
  std::vector<uint16_t> elements;

  // The outer radius is 1 so the mesh scales like the other unit primitives.
  constexpr float outerRadius = 1.0F - TORUS_INNER_RADIUS;

  vertices.reserve(static_cast<size_t>(TORUS_RINGS + 1) * static_cast<size_t>(TORUS_SIDES + 1));
  elements.reserve(static_cast<size_t>(TORUS_RINGS) * static_cast<size_t>(TORUS_SIDES) * 6);

  for(int ring = 0; ring <= TORUS_RINGS; ++ring) {
    const float u = static_cast<float>(ring) / static_cast<float>(TORUS_RINGS) * 2.0F * PI;
    const float cosU = std::cos(u);
    const float sinU = std::sin(u);

    for(int side = 0; side <= TORUS_SIDES; ++side) {
      const float v = static_cast<float>(side) / static_cast<float>(TORUS_SIDES) * 2.0F * PI;
      const float cosV = std::cos(v);
      const float sinV = std::sin(v);

      const glm::vec3 normal = {cosU * cosV, sinU * cosV, sinV};
      const glm::vec3 position = {
        cosU * (outerRadius + TORUS_INNER_RADIUS * cosV), sinU * (outerRadius + TORUS_INNER_RADIUS * cosV), TORUS_INNER_RADIUS * sinV};
      vertices.push_back({position, normal});
    }
  }

  const auto verticesPerRing = TORUS_SIDES + 1;

  for(int ring = 0; ring < TORUS_RINGS; ++ring) {
    for(int side = 0; side < TORUS_SIDES; ++side) {
      const auto i0 = static_cast<uint16_t>((ring * verticesPerRing) + side);
      const auto i1 = static_cast<uint16_t>(((ring + 1) * verticesPerRing) + side);
      const auto i2 = static_cast<uint16_t>(((ring + 1) * verticesPerRing) + side + 1);
      const auto i3 = static_cast<uint16_t>((ring * verticesPerRing) + side + 1);

      elements.push_back(i0);
      elements.push_back(i1);
      elements.push_back(i2);

      elements.push_back(i0);
      elements.push_back(i2);
      elements.push_back(i3);
    }
  }

  g_objectIndexCount = static_cast<GLsizei>(elements.size());
  g_objectIndexType = GL_UNSIGNED_SHORT;
  uploadMesh(g_objectVAO, g_objectVBO, g_objectEBO, vertices, elements);
}

///////////////////////////////////////////////////////////////////////////////
// the object rotated by the trackball: OBJECT_FILE if it is present, otherwise
// the generated torus above
///////////////////////////////////////////////////////////////////////////////
void createObjectBuffers() {
  g_objectModelLoaded = g_objectModel.import(OBJECT_FILE);

  if(!g_objectModelLoaded) {
    createTorusBuffers();
    return;
  }

  g_objectModel.normalize(1.0F);  // centered, unit bounding-sphere radius

  g_objectIndexCount = static_cast<GLsizei>(g_objectModel.getNumberOfIndices());
  g_objectIndexType = GL_UNSIGNED_INT;
  uploadObjModel(g_objectVAO, g_objectVBO, g_objectEBO, g_objectModel);
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

///////////////////////////////////////////////////////////////////////////////
// the three coloured axis lines, in unit space so they can be scaled
///////////////////////////////////////////////////////////////////////////////
void createAxisBuffers() {
  const std::array<FlatVertex, 6> axis = {
    FlatVertex{{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F, 1.0F}},
    FlatVertex{{1.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F, 1.0F}},
    FlatVertex{{0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F, 1.0F}},
    FlatVertex{{0.0F, 1.0F, 0.0F}, {0.0F, 1.0F, 0.0F, 1.0F}},
    FlatVertex{{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F, 1.0F}},
    FlatVertex{{0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F, 1.0F}},
  };

  glGenVertexArrays(1, &g_axisLinesVAO);
  glBindVertexArray(g_axisLinesVAO);

  glCreateBuffers(1, &g_axisLinesVBO);
  glNamedBufferStorage(g_axisLinesVBO, static_cast<GLsizeiptr>(axis.size() * sizeof(FlatVertex)), axis.data(), GL_DYNAMIC_STORAGE_BIT);

  setupFlatFormat(g_axisLinesVBO);
}

///////////////////////////////////////////////////////////////////////////////
// one scratch buffer, refilled per draw, for the path/vectors/circle
///////////////////////////////////////////////////////////////////////////////
void createDynamicBuffers() {
  // the circle is the largest of the three
  constexpr size_t capacity = CIRCLE_STEPS + 1;

  glGenVertexArrays(1, &g_dynamicVAO);
  glBindVertexArray(g_dynamicVAO);

  glCreateBuffers(1, &g_dynamicVBO);
  glNamedBufferStorage(g_dynamicVBO, static_cast<GLsizeiptr>(capacity * sizeof(FlatVertex)), nullptr, GL_DYNAMIC_STORAGE_BIT);

  setupFlatFormat(g_dynamicVBO);
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

  uniform vec4 uColor;
  uniform vec3 uLightPos;

  layout(location=0) out vec4 out_Color;

  void main() {
    // Matches the fixed function setup of the original: GL_COLOR_MATERIAL
    // tracking ambient and diffuse from the vertex colour, a light with no
    // ambient and a white diffuse and specular, and a shininess of 128.
    vec3 normal = normalize(IN.esNormal);
    vec3 light = normalize(uLightPos - IN.esVertex);
    vec3 view = normalize(-IN.esVertex);
    vec3 halfv = normalize(light + view);

    float dotNL = max(dot(normal, light), 0.0);
    float dotNH = max(dot(normal, halfv), 0.0);

    vec3 color = uColor.rgb * dotNL + vec3(1.0) * pow(dotNH, 128.0);

    out_Color = vec4(color, uColor.a);
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
  g_uLitColor = glGetUniformLocation(g_litProgram, "uColor");
  g_uLitLightPos = glGetUniformLocation(g_litProgram, "uLightPos");
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
  if(g_sphereVAO) {
    glDeleteVertexArrays(1, &g_sphereVAO);
    g_sphereVAO = 0;
  }
  if(g_sphereVBO) {
    glDeleteBuffers(1, &g_sphereVBO);
    g_sphereVBO = 0;
  }
  if(g_sphereEBO) {
    glDeleteBuffers(1, &g_sphereEBO);
    g_sphereEBO = 0;
  }

  if(g_objectVAO) {
    glDeleteVertexArrays(1, &g_objectVAO);
    g_objectVAO = 0;
  }
  if(g_objectVBO) {
    glDeleteBuffers(1, &g_objectVBO);
    g_objectVBO = 0;
  }
  if(g_objectEBO) {
    glDeleteBuffers(1, &g_objectEBO);
    g_objectEBO = 0;
  }

  if(g_axisLinesVAO) {
    glDeleteVertexArrays(1, &g_axisLinesVAO);
    g_axisLinesVAO = 0;
  }
  if(g_axisLinesVBO) {
    glDeleteBuffers(1, &g_axisLinesVBO);
    g_axisLinesVBO = 0;
  }

  if(g_dynamicVAO) {
    glDeleteVertexArrays(1, &g_dynamicVAO);
    g_dynamicVAO = 0;
  }
  if(g_dynamicVBO) {
    glDeleteBuffers(1, &g_dynamicVBO);
    g_dynamicVBO = 0;
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
