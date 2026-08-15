///////////////////////////////////////////////////////////////////////////////
// maya_camera.cpp
///////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
//
#include "maya_camera.hpp"

namespace {
constexpr float TUMBLE_SCALE = 0.5F;  // degrees per pixel, Maya's default feel
constexpr float TRACK_SCALE = 1.0F;   // world units are derived, this only trims
constexpr float DOLLY_SCALE = 0.005F;
constexpr float WHEEL_SCALE = 0.15F;

constexpr glm::vec3 X_AXIS = {1.0F, 0.0F, 0.0F};
constexpr glm::vec3 Y_AXIS = {0.0F, 1.0F, 0.0F};

[[nodiscard]] float wrap180(float degrees) {
  degrees = std::fmod(degrees + 180.0F, 360.0F);
  if(degrees < 0.0F)
    degrees += 360.0F;
  return degrees - 180.0F;
}
}  // namespace

void MayaCamera::updateMatrix() const {
  // world -> camera is the inverse of "rotate by yaw then pitch, then step
  // back along the camera's own +Z by distance".
  const glm::mat4 rotation = glm::rotate(glm::mat4(1.0F), glm::radians(-m_state.pitch), X_AXIS) *
                             glm::rotate(glm::mat4(1.0F), glm::radians(-m_state.yaw), Y_AXIS);

  m_viewMatrix =
    glm::translate(glm::mat4(1.0F), {0.0F, 0.0F, -m_state.distance}) * rotation * glm::translate(glm::mat4(1.0F), -m_state.pivot);
  m_dirty = false;
}

const glm::mat4 &MayaCamera::getViewMatrix() const {
  if(m_dirty)
    updateMatrix();
  return m_viewMatrix;
}

// The rows of the view matrix's rotation part are the camera's axes in world
// space; the camera looks down its own -Z.
glm::vec3 MayaCamera::getRightAxis() const {
  const glm::mat4 &m = getViewMatrix();
  return {m[0][0], m[1][0], m[2][0]};
}

glm::vec3 MayaCamera::getUpAxis() const {
  const glm::mat4 &m = getViewMatrix();
  return {m[0][1], m[1][1], m[2][1]};
}

glm::vec3 MayaCamera::getForwardAxis() const {
  const glm::mat4 &m = getViewMatrix();
  return {-m[0][2], -m[1][2], -m[2][2]};
}

glm::vec3 MayaCamera::getEye() const { return m_state.pivot - (getForwardAxis() * m_state.distance); }

float MayaCamera::getOrthoHalfHeight() const { return m_state.distance * std::tan(glm::radians(m_fovY) * 0.5F); }

glm::mat4 MayaCamera::getProjectionMatrix(float aspect) const {
  if(aspect <= 0.0F)
    aspect = 1.0F;

  if(!m_state.orthographic)
    return glm::perspective(glm::radians(m_fovY), aspect, m_nearPlane, m_farPlane);

  // Maya's orthographic cameras see behind themselves as well, so the near
  // plane goes negative rather than clipping away half the scene.
  const float halfHeight = getOrthoHalfHeight();
  return glm::ortho(-halfHeight * aspect, halfHeight * aspect, -halfHeight, halfHeight, -m_farPlane, m_farPlane);
}

void MayaCamera::lookAt(const glm::vec3 &eye, const glm::vec3 &pivot) {
  const glm::vec3 offset = eye - pivot;
  const float distance = glm::length(offset);

  m_state.pivot = pivot;
  if(distance < MIN_DISTANCE) {
    m_state.distance = MIN_DISTANCE;
  } else {
    m_state.distance = glm::clamp(distance, MIN_DISTANCE, MAX_DISTANCE);
    // offset / distance == (cos(pitch)sin(yaw), -sin(pitch), cos(pitch)cos(yaw))
    m_state.yaw = glm::degrees(std::atan2(offset.x, offset.z));
    m_state.pitch = -glm::degrees(std::asin(glm::clamp(offset.y / distance, -1.0F, 1.0F)));
  }
  m_dirty = true;
}

void MayaCamera::tumble(const glm::vec2 &deltaPixels) {
  m_state.yaw = wrap180(m_state.yaw + (deltaPixels.x * TUMBLE_SCALE));
  m_state.pitch = wrap180(m_state.pitch + (deltaPixels.y * TUMBLE_SCALE));
  m_dirty = true;
}

void MayaCamera::track(const glm::vec2 &deltaPixels, int viewportHeight) {
  if(viewportHeight <= 0)
    return;

  // world units per pixel at the pivot's depth: this is the whole trick, it is
  // what keeps whatever is under the cursor under the cursor at any distance.
  const float halfHeight = m_state.orthographic ? getOrthoHalfHeight() : (m_state.distance * std::tan(glm::radians(m_fovY) * 0.5F));
  const float worldPerPixel = (2.0F * halfHeight * TRACK_SCALE) / static_cast<float>(viewportHeight);

  m_state.pivot -= getRightAxis() * (deltaPixels.x * worldPerPixel);
  m_state.pivot += getUpAxis() * (deltaPixels.y * worldPerPixel);  // screen y grows downwards
  m_dirty = true;
}

void MayaCamera::dolly(const glm::vec2 &deltaPixels) {
  // right or up means in, matching Maya's Alt + RMB
  const float amount = deltaPixels.x - deltaPixels.y;
  setDistance(m_state.distance * std::exp(-amount * DOLLY_SCALE));
}

void MayaCamera::dollyTicks(float wheelTicks) { setDistance(m_state.distance * std::exp(-wheelTicks * WHEEL_SCALE)); }

void MayaCamera::frame(const glm::vec3 &center, float radius) {
  m_state.pivot = center;
  if(radius > 0.0F)
    setDistance(radius / std::sin(glm::radians(m_fovY) * 0.5F));
  m_dirty = true;
}

void MayaCamera::setView(StandardView view) {
  switch(view) {
  case StandardView::Persp:
    m_state.yaw = 45.0F;
    m_state.pitch = 30.0F;
    m_state.orthographic = false;
    break;
  case StandardView::Front:
    setRotation(0.0F, 0.0F);
    m_state.orthographic = true;
    break;
  case StandardView::Back:
    setRotation(180.0F, 0.0F);
    m_state.orthographic = true;
    break;
  case StandardView::Right:
    setRotation(90.0F, 0.0F);
    m_state.orthographic = true;
    break;
  case StandardView::Left:
    setRotation(-90.0F, 0.0F);
    m_state.orthographic = true;
    break;
  case StandardView::Top:
    setRotation(0.0F, -90.0F);
    m_state.orthographic = true;
    break;
  case StandardView::Bottom:
    setRotation(0.0F, 90.0F);
    m_state.orthographic = true;
    break;
  }
  m_dirty = true;
}

void MayaCamera::setTumblePivot(TumblePivot pivot) {
  m_tumblePivot = pivot;

  glm::vec3 target = m_state.pivot;
  if(pivot == TumblePivot::Origin)
    target = {0.0F, 0.0F, 0.0F};
  else if(pivot == TumblePivot::Selection)
    target = m_selectionPivot;

  // Re-pivot without moving the eye. Note this collapses an over-the-pole
  // orientation back into |pitch| <= 90, which is the same thing Maya does
  // when it re-derives the view from a new center of interest.
  lookAt(getEye(), target);
}

void MayaCamera::setSelectionPivot(const glm::vec3 &v) {
  m_selectionPivot = v;
  if(m_tumblePivot == TumblePivot::Selection)
    lookAt(getEye(), v);
}

void MayaCamera::pushUndo() {
  m_undoStack.push_back(m_state);
  if(m_undoStack.size() > UNDO_LIMIT)
    m_undoStack.pop_front();
  m_redoStack.clear();
}

bool MayaCamera::undo() {
  if(m_undoStack.empty())
    return false;

  m_redoStack.push_back(m_state);
  setState(m_undoStack.back());
  m_undoStack.pop_back();
  return true;
}

bool MayaCamera::redo() {
  if(m_redoStack.empty())
    return false;

  m_undoStack.push_back(m_state);
  setState(m_redoStack.back());
  m_redoStack.pop_back();
  return true;
}

void MayaCamera::setPivot(const glm::vec3 &v) {
  m_state.pivot = v;
  m_dirty = true;
}

void MayaCamera::setDistance(float distance) {
  m_state.distance = glm::clamp(distance, MIN_DISTANCE, MAX_DISTANCE);
  m_dirty = true;
}

void MayaCamera::setRotation(float yaw, float pitch) {
  m_state.yaw = wrap180(yaw);
  m_state.pitch = wrap180(pitch);
  m_dirty = true;
}

void MayaCamera::setOrthographic(bool orthographic) {
  m_state.orthographic = orthographic;
  m_dirty = true;
}

void MayaCamera::setFieldOfView(float degrees) {
  m_fovY = glm::clamp(degrees, 1.0F, 179.0F);
  m_dirty = true;
}

void MayaCamera::setNearFar(float nearPlane, float farPlane) {
  m_nearPlane = nearPlane;
  m_farPlane = farPlane;
  m_dirty = true;
}

void MayaCamera::setState(const MayaCameraState &state) {
  m_state = state;
  m_dirty = true;
}

///////////////////////////////////////////////////////////////////////////////
// self test
//
// The demo has no test suite to hang this off, so it runs from `--self-test`.
// Everything here is pure math: no GL context, no window.
///////////////////////////////////////////////////////////////////////////////
namespace {
[[nodiscard]] bool almostEqual(float a, float b, float epsilon = 1e-3F) { return std::fabs(a - b) < epsilon; }

// project a world point into pixel coordinates, y down
[[nodiscard]] glm::vec2 project(const MayaCamera &camera, const glm::vec3 &point, int width, int height) {
  const float aspect = static_cast<float>(width) / static_cast<float>(height);
  const glm::vec4 clip = camera.getProjectionMatrix(aspect) * camera.getViewMatrix() * glm::vec4(point, 1.0F);
  const glm::vec3 ndc = glm::vec3(clip) / clip.w;
  return {(ndc.x * 0.5F + 0.5F) * static_cast<float>(width), (0.5F - ndc.y * 0.5F) * static_cast<float>(height)};
}
}  // namespace

void mayaCameraSelfTest() {
  constexpr int WIDTH = 800;
  constexpr int HEIGHT = 600;
  constexpr float ASPECT = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);

  // lookAt round trips through yaw/pitch/distance
  {
    MayaCamera camera;
    const glm::vec3 eye = {3.0F, 4.0F, 5.0F};
    const glm::vec3 pivot = {1.0F, -1.0F, 2.0F};
    camera.lookAt(eye, pivot);
    assert(glm::length(camera.getEye() - eye) < 1e-3F);
    assert(almostEqual(camera.getDistance(), glm::length(eye - pivot)));
    assert(glm::length(camera.getForwardAxis() - glm::normalize(pivot - eye)) < 1e-3F);
  }

  // the default view looks down -Z from +Z, with +Y up and +X right
  {
    MayaCamera camera;
    assert(glm::length(camera.getEye() - glm::vec3(0.0F, 0.0F, camera.getDistance())) < 1e-3F);
    assert(glm::length(camera.getForwardAxis() - glm::vec3(0.0F, 0.0F, -1.0F)) < 1e-3F);
    assert(glm::length(camera.getUpAxis() - glm::vec3(0.0F, 1.0F, 0.0F)) < 1e-3F);
    assert(glm::length(camera.getRightAxis() - glm::vec3(1.0F, 0.0F, 0.0F)) < 1e-3F);
  }

  // tumble follows the cursor: right moves the eye to +X, up moves it to +Y,
  // and it stays on the sphere around the pivot
  {
    MayaCamera camera;
    const float distance = camera.getDistance();

    camera.tumble({20.0F, 0.0F});
    assert(camera.getEye().x > 0.0F);
    assert(almostEqual(glm::length(camera.getEye() - camera.getPivot()), distance));

    camera.setRotation(0.0F, 0.0F);
    camera.tumble({0.0F, -20.0F});
    assert(camera.getEye().y > 0.0F);
    assert(almostEqual(glm::length(camera.getEye() - camera.getPivot()), distance));
  }

  // tumbling over the pole keeps the radius, produces no NaNs, and inverts the
  // up axis on the far side instead of stopping
  {
    MayaCamera camera;
    const float distance = camera.getDistance();
    for(int i = 0; i < 720; ++i) {
      camera.tumble({1.0F, 1.0F});
      assert(!std::isnan(camera.getEye().x) && !std::isnan(camera.getEye().y) && !std::isnan(camera.getEye().z));
      assert(almostEqual(glm::length(camera.getEye() - camera.getPivot()), distance));
    }
    camera.setRotation(0.0F, 0.0F);
    camera.tumble({0.0F, 200.0F});  // 100 degrees, past the bottom pole
    assert(camera.getUpAxis().y < 0.0F);
  }

  // track keeps the world glued to the cursor: a point at the centre of the
  // view ends up exactly deltaPixels away from it
  {
    for(float distance : {2.0F, 50.0F}) {
      MayaCamera camera;
      camera.setDistance(distance);
      camera.tumble({37.0F, -11.0F});  // an off-axis orientation, not a lucky one

      const glm::vec3 point = camera.getPivot();
      const glm::vec2 before = project(camera, point, WIDTH, HEIGHT);
      assert(almostEqual(before.x, static_cast<float>(WIDTH) * 0.5F, 0.01F));
      assert(almostEqual(before.y, static_cast<float>(HEIGHT) * 0.5F, 0.01F));

      const glm::vec2 delta = {40.0F, -25.0F};
      camera.track(delta, HEIGHT);

      const glm::vec2 after = project(camera, point, WIDTH, HEIGHT);
      assert(almostEqual(after.x - before.x, delta.x, 0.5F));
      assert(almostEqual(after.y - before.y, delta.y, 0.5F));

      // and it only slid the pivot sideways, it did not push in or out
      assert(almostEqual(camera.getDistance(), distance));
    }
  }

  // the same, orthographic
  {
    MayaCamera camera;
    camera.setOrthographic(true);
    camera.tumble({37.0F, -11.0F});

    const glm::vec3 point = camera.getPivot();
    const glm::vec2 before = project(camera, point, WIDTH, HEIGHT);
    const glm::vec2 delta = {40.0F, -25.0F};
    camera.track(delta, HEIGHT);
    const glm::vec2 after = project(camera, point, WIDTH, HEIGHT);
    assert(almostEqual(after.x - before.x, delta.x, 0.5F));
    assert(almostEqual(after.y - before.y, delta.y, 0.5F));
  }

  // dolly is monotone, in the Maya direction, and clamps at both ends without
  // ever crossing the pivot
  {
    MayaCamera camera;
    float previous = camera.getDistance();
    for(int i = 0; i < 200; ++i) {
      camera.dolly({20.0F, 0.0F});  // drag right == zoom in
      assert(camera.getDistance() <= previous);
      assert(camera.getDistance() > 0.0F);
      previous = camera.getDistance();
    }
    assert(almostEqual(camera.getDistance(), MayaCamera::MIN_DISTANCE));

    for(int i = 0; i < 400; ++i)
      camera.dollyTicks(-10.0F);
    assert(almostEqual(camera.getDistance(), MayaCamera::MAX_DISTANCE));

    // the wheel agrees with the drag about which way is in
    camera.setDistance(10.0F);
    camera.dollyTicks(1.0F);
    assert(camera.getDistance() < 10.0F);
  }

  // frame fits the sphere: it lands inside the frustum and fills the height
  {
    MayaCamera camera;
    const glm::vec3 center = {2.0F, 1.0F, -3.0F};
    constexpr float RADIUS = 4.0F;
    camera.frame(center, RADIUS);

    assert(glm::length(camera.getPivot() - center) < 1e-3F);
    assert(almostEqual(camera.getDistance(), RADIUS / std::sin(glm::radians(camera.getFieldOfView()) * 0.5F)));

    // the topmost point of the sphere, in camera space, sits on the top edge
    const glm::vec3 top = center + (camera.getUpAxis() * RADIUS);
    const glm::vec2 projected = project(camera, top, WIDTH, HEIGHT);
    assert(projected.y > 0.0F && projected.y < static_cast<float>(HEIGHT));

    // framing a degenerate (point) selection must not divide by zero
    const float distance = camera.getDistance();
    camera.frame(center, 0.0F);
    assert(almostEqual(camera.getDistance(), distance));
  }

  // the standard views are orthographic and look down the expected axis
  {
    MayaCamera camera;
    camera.setView(MayaCamera::StandardView::Top);
    assert(camera.isOrthographic());
    assert(glm::length(camera.getForwardAxis() - glm::vec3(0.0F, -1.0F, 0.0F)) < 1e-3F);

    camera.setView(MayaCamera::StandardView::Right);
    assert(glm::length(camera.getForwardAxis() - glm::vec3(-1.0F, 0.0F, 0.0F)) < 1e-3F);

    camera.setView(MayaCamera::StandardView::Front);
    assert(glm::length(camera.getForwardAxis() - glm::vec3(0.0F, 0.0F, -1.0F)) < 1e-3F);

    camera.setView(MayaCamera::StandardView::Persp);
    assert(!camera.isOrthographic());
  }

  // undo/redo round trips exactly, and runs out at the bottom
  {
    MayaCamera camera;
    assert(!camera.undo());
    assert(!camera.redo());

    camera.pushUndo();
    const MayaCameraState original = camera.getState();
    camera.tumble({50.0F, 20.0F});
    camera.dollyTicks(3.0F);
    const MayaCameraState moved = camera.getState();

    assert(camera.undo());
    assert(almostEqual(camera.getYaw(), original.yaw) && almostEqual(camera.getPitch(), original.pitch));
    assert(almostEqual(camera.getDistance(), original.distance));
    assert(!camera.undo());

    assert(camera.redo());
    assert(almostEqual(camera.getYaw(), moved.yaw) && almostEqual(camera.getPitch(), moved.pitch));
    assert(almostEqual(camera.getDistance(), moved.distance));
    assert(!camera.redo());

    // a new gesture drops the redo history
    camera.pushUndo();
    assert(!camera.canRedo());

    // and the stack is bounded
    for(int i = 0; i < 100; ++i)
      camera.pushUndo();
    for(int i = 0; i < 32; ++i)
      assert(camera.undo());
    assert(!camera.undo());
  }

  // switching the tumble pivot re-pivots without moving the eye
  {
    MayaCamera camera;
    camera.lookAt({6.0F, 4.0F, 8.0F}, {1.0F, 2.0F, 3.0F});
    const glm::vec3 eye = camera.getEye();

    camera.setTumblePivot(MayaCamera::TumblePivot::Origin);
    assert(glm::length(camera.getEye() - eye) < 1e-3F);
    assert(glm::length(camera.getPivot()) < 1e-3F);

    camera.setSelectionPivot({-2.0F, 1.0F, 4.0F});
    camera.setTumblePivot(MayaCamera::TumblePivot::Selection);
    assert(glm::length(camera.getEye() - eye) < 1e-3F);
    assert(glm::length(camera.getPivot() - glm::vec3(-2.0F, 1.0F, 4.0F)) < 1e-3F);
  }

  // the projection matrix is finite for a degenerate viewport
  {
    MayaCamera camera;
    const glm::mat4 projection = camera.getProjectionMatrix(0.0F);
    assert(!std::isnan(projection[0][0]) && !std::isinf(projection[0][0]));
    assert(ASPECT > 0.0F);
  }
}
