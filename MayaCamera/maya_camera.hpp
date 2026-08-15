///////////////////////////////////////////////////////////////////////////////
// maya_camera.hpp
// ===============
// Viewport camera with Autodesk Maya's navigation model.
//
// The camera is an eye orbiting a "center of interest" (the pivot). Everything
// it does is one of five verbs:
//
//   tumble  Alt + LMB   orbit around the pivot, over the poles included
//   track   Alt + MMB   slide the pivot in the view plane, world sticks to the
//                       cursor because the step scales with distance and fov
//   dolly   Alt + RMB   exponential in/out, slows down near the pivot and
//                       never crosses it
//   frame   F / A       fit a bounding sphere to the vertical field of view
//   view    1..6 / 0    snap to the six orthographic bookmarks, or back to
//                       perspective
//
// plus view undo/redo on Maya's [ and ] keys.
//
// Angles are degrees, the world is Y-up and right-handed, and the camera looks
// down its own -Z the way OpenGL expects. No animation, no smoothing: Maya's
// viewport moves the instant the mouse does.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <deque>
#include <glm/glm.hpp>

// One snapshot of the camera, and the unit the undo/redo stack stores.
struct MayaCameraState {
  glm::vec3 pivot = {0.0F, 0.0F, 0.0F};  // center of interest
  float yaw = 0.0F;                      // degrees around world +Y
  float pitch = 0.0F;                    // degrees around camera-local +X
  float distance = 5.0F;                 // eye to pivot
  bool orthographic = false;
};

class MayaCamera final {
public:
  enum class StandardView { Persp, Front, Back, Left, Right, Top, Bottom };

  // Where tumble rotates around. Maya calls this the tumble pivot.
  enum class TumblePivot { CenterOfInterest, Origin, Selection };

  MayaCamera() = default;
  MayaCamera(const glm::vec3 &eye, const glm::vec3 &pivot) { lookAt(eye, pivot); }

  // point the camera at a pivot from a given eye position
  void lookAt(const glm::vec3 &eye, const glm::vec3 &pivot);

  // Alt + LMB. Positive x drags the camera right, positive y (screen down)
  // drags it down; both follow the cursor. Deliberately unclamped: Maya keeps
  // world +Y as up, so tumbling over a pole flips the image rather than
  // stopping, and that is the behaviour being reproduced here.
  void tumble(const glm::vec2 &deltaPixels);

  // Alt + MMB. viewportHeight is what makes the world stay glued to the
  // cursor -- the step is the world size of one pixel at the pivot's depth.
  void track(const glm::vec2 &deltaPixels, int viewportHeight);

  // Alt + RMB. Right and up dolly in. Exponential, so it decelerates as it
  // approaches and clamps instead of punching through the pivot.
  void dolly(const glm::vec2 &deltaPixels);
  void dollyTicks(float wheelTicks);

  // F and A: fit a bounding sphere to the vertical field of view.
  void frame(const glm::vec3 &center, float radius);

  // the six orthographic bookmarks, and back to the perspective camera
  void setView(StandardView view);

  // Re-pivots immediately, keeping the eye where it is so the view does not
  // jump. Feed Selection with setSelectionPivot().
  void setTumblePivot(TumblePivot pivot);
  void setSelectionPivot(const glm::vec3 &v);

  // Maya's [ and ]. The demo pushes one entry per gesture, so one undo steps
  // back one drag.
  void pushUndo();
  bool undo();
  bool redo();
  [[nodiscard]] bool canUndo() const { return !m_undoStack.empty(); }
  [[nodiscard]] bool canRedo() const { return !m_redoStack.empty(); }
  [[nodiscard]] size_t undoDepth() const { return m_undoStack.size(); }
  [[nodiscard]] size_t redoDepth() const { return m_redoStack.size(); }

  void setPivot(const glm::vec3 &v);
  void setDistance(float distance);
  void setRotation(float yaw, float pitch);
  void setOrthographic(bool orthographic);
  void setFieldOfView(float degrees);
  void setNearFar(float nearPlane, float farPlane);
  void setState(const MayaCameraState &state);

  [[nodiscard]] const glm::mat4 &getViewMatrix() const;
  [[nodiscard]] glm::mat4 getProjectionMatrix(float aspect) const;

  [[nodiscard]] glm::vec3 getEye() const;
  [[nodiscard]] glm::vec3 getRightAxis() const;
  [[nodiscard]] glm::vec3 getUpAxis() const;
  [[nodiscard]] glm::vec3 getForwardAxis() const;

  [[nodiscard]] const glm::vec3 &getPivot() const { return m_state.pivot; }
  [[nodiscard]] float getYaw() const { return m_state.yaw; }
  [[nodiscard]] float getPitch() const { return m_state.pitch; }
  [[nodiscard]] float getDistance() const { return m_state.distance; }
  [[nodiscard]] bool isOrthographic() const { return m_state.orthographic; }
  [[nodiscard]] float getFieldOfView() const { return m_fovY; }
  [[nodiscard]] float getNearPlane() const { return m_nearPlane; }
  [[nodiscard]] float getFarPlane() const { return m_farPlane; }
  [[nodiscard]] const MayaCameraState &getState() const { return m_state; }
  [[nodiscard]] TumblePivot getTumblePivot() const { return m_tumblePivot; }

  // half height of the orthographic view volume at the pivot's depth
  [[nodiscard]] float getOrthoHalfHeight() const;

  static constexpr float MIN_DISTANCE = 0.01F;
  static constexpr float MAX_DISTANCE = 10000.0F;

private:
  void updateMatrix() const;

  MayaCameraState m_state;
  TumblePivot m_tumblePivot = TumblePivot::CenterOfInterest;
  glm::vec3 m_selectionPivot = {0.0F, 0.0F, 0.0F};

  float m_fovY = 54.43F;  // Maya's default 35mm lens
  float m_nearPlane = 0.1F;
  float m_farPlane = 1000.0F;

  // ponytail: 32 snapshots of view history, deep enough for a session of
  // navigating. Swap the deques for a command stack if it ever has to hold
  // anything but whole-camera states.
  static constexpr size_t UNDO_LIMIT = 32;
  std::deque<MayaCameraState> m_undoStack;
  std::deque<MayaCameraState> m_redoStack;

  mutable glm::mat4 m_viewMatrix = glm::mat4(1.0F);
  mutable bool m_dirty = true;
};

// assert-based check of the camera math, run by the demo's --self-test flag
void mayaCameraSelfTest();
