//-----------------------------------------------------------------------------
// Copyright (c) 2007-2008 dhpoware. All Rights Reserved.
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

#pragma once
#include <SDL2/SDL.h>
#include <glm/glm.hpp>

class Keyboard final {
public:
  static Keyboard &instance();

  [[nodiscard]] bool keyDown(SDL_Scancode key) const { return m_pCurrKeyStates[key] != 0; }

  [[nodiscard]] bool keyUp(SDL_Scancode key) const { return m_pCurrKeyStates[key] == 0; }

  [[nodiscard]] bool keyPressed(SDL_Scancode key) const { return ((m_pCurrKeyStates[key] != 0) && (m_pPrevKeyStates[key] == 0)); }

  void update();

private:
  Keyboard();

  uint8_t m_keyStates[2][SDL_NUM_SCANCODES];
  uint8_t *m_pCurrKeyStates = m_keyStates[0];
  uint8_t *m_pPrevKeyStates = m_keyStates[1];
};

class Mouse final {
public:
  enum MouseButton { BUTTON_LEFT = 0, BUTTON_RIGHT = 1, BUTTON_MIDDLE = 2 };

  static Mouse &instance();

  Mouse(const Mouse &) = delete;
  Mouse(Mouse &&) = delete;
  Mouse &operator=(const Mouse &) = delete;
  Mouse &operator=(Mouse &&) = delete;

  [[nodiscard]] bool buttonDown(MouseButton button) const { return m_pCurrButtonStates[button]; }

  [[nodiscard]] bool buttonPressed(MouseButton button) const {
    return m_pCurrButtonStates[button] && !m_pPrevButtonStates[button];
  }

  [[nodiscard]] bool buttonUp(MouseButton button) const { return !m_pCurrButtonStates[button]; }

  [[nodiscard]] bool cursorIsVisible() const { return m_cursorVisible; }

  [[nodiscard]] bool isMouseSmoothing() const { return m_enableFiltering; }

  [[nodiscard]] float xDistanceFromWindowCenter() const { return m_ptDistFromWindowCenter.x; }

  [[nodiscard]] float yDistanceFromWindowCenter() const { return m_ptDistFromWindowCenter.y; }

  [[nodiscard]] int xPos() const { return m_ptCurrentPos.x; }

  [[nodiscard]] int yPos() const { return m_ptCurrentPos.y; }

  [[nodiscard]] float weightModifier() const { return m_weightModifier; }

  [[nodiscard]] float wheelPos() const { return m_mouseWheel; }

  bool attach(SDL_Window *window);
  void detach();
  void hideCursor(bool hide);
  void handleMsg(int delta);
  void moveTo(glm::ivec2 pos);
  void moveToWindowCenter();
  void setWeightModifier(float weightModifier);
  void smoothMouse(bool smooth);
  void update();

private:
  Mouse();
  ~Mouse();

  void performMouseFiltering(glm::vec2 pos);

  static const float DEFAULT_WEIGHT_MODIFIER;
  static const int HISTORY_BUFFER_SIZE = 10;

  SDL_Window *m_window = nullptr;
  int m_historyBufferSize = HISTORY_BUFFER_SIZE;
  int m_wheelDelta = 0;
  int m_prevWheelDelta = 0;
  float m_mouseWheel = 0.0F;
  glm::vec2 m_ptDistFromWindowCenter = {0, 0};
  float m_weightModifier = DEFAULT_WEIGHT_MODIFIER;
  float m_filtered[2] = {0, 0};
  float m_history[HISTORY_BUFFER_SIZE * 2];
  bool m_moveToWindowCenterPending = false;
  bool m_enableFiltering = true;
  bool m_cursorVisible = true;
  bool m_buttonStates[2][3];
  bool *m_pCurrButtonStates = m_buttonStates[0];
  bool *m_pPrevButtonStates = m_buttonStates[1];
  glm::ivec2 m_ptWindowCenterPos = {0, 0};
  glm::ivec2 m_ptCurrentPos = {0, 0};
};
