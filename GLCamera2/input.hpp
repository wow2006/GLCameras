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

#include <glm/vec2.hpp>

class Keyboard {
public:
  static Keyboard &instance();

  bool keyDown(SDL_Scancode key) const { return 1 == m_pCurrKeyStates[key]; }

  bool keyUp(SDL_Scancode key) const { return 1 == m_pCurrKeyStates[key]; }

  bool keyPressed(SDL_Scancode key) const { return (1 == m_pCurrKeyStates[key]) && !(1 == m_pPrevKeyStates[key]); }

  void update();

private:
  Keyboard();
  ~Keyboard();

  uint8_t m_keyStates[2][256] = {};
  uint8_t *m_pCurrKeyStates = m_keyStates[0];
  uint8_t *m_pPrevKeyStates = m_keyStates[1];
};

class Mouse {
public:
  enum MouseButton { BUTTON_LEFT = 0, BUTTON_RIGHT = 1, BUTTON_MIDDLE = 2 };

  static Mouse &instance();

  bool buttonDown(MouseButton button) const { return m_pCurrButtonStates[button]; }

  bool buttonPressed(MouseButton button) const { return m_pCurrButtonStates[button] && !m_pPrevButtonStates[button]; }

  bool buttonUp(MouseButton button) const { return !m_pCurrButtonStates[button]; }

  bool cursorIsVisible() const { return m_cursorVisible; }

  bool isMouseSmoothing() const { return m_enableFiltering; }

  float xDistanceFromWindowCenter() const { return m_ptDistFromWindowCenter.x; }

  float yDistanceFromWindowCenter() const { return m_ptDistFromWindowCenter.y; }

  int xPos() const { return m_ptCurrentPos.x; }

  int yPos() const { return m_ptCurrentPos.y; }

  float weightModifier() const { return m_weightModifier; }

  float wheelPos() const { return m_mouseWheel; }

  bool attach(SDL_Window *m_window);
  void detach();
  void handleMsg(int delta);
  void hideCursor(bool hide);
  void moveTo(uint32_t x, uint32_t y);
  void moveToWindowCenter();
  void setWeightModifier(float weightModifier);
  void smoothMouse(bool smooth);
  void update();

private:
  Mouse();
  Mouse(const Mouse &);
  Mouse &operator=(const Mouse &);
  ~Mouse();

  void performMouseFiltering(float x, float y);

  static const float DEFAULT_WEIGHT_MODIFIER;
  static const int HISTORY_BUFFER_SIZE = 10;

  SDL_Window *m_window = nullptr;
  int m_historyBufferSize;
  int m_wheelDelta;
  int m_prevWheelDelta;
  float m_mouseWheel;
  glm::vec2 m_ptDistFromWindowCenter;
  float m_weightModifier;
  float m_filtered[2];
  float m_history[HISTORY_BUFFER_SIZE * 2];
  bool m_moveToWindowCenterPending;
  bool m_enableFiltering;
  bool m_cursorVisible;
  bool m_buttonStates[2][3];
  bool *m_pCurrButtonStates;
  bool *m_pPrevButtonStates;
  glm::ivec2 m_ptWindowCenterPos;
  glm::ivec2 m_ptCurrentPos;
};