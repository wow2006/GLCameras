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
// Internal
#include "input.hpp"
#include <SDL2/SDL.h>

//-----------------------------------------------------------------------------
// Keyboard.
//-----------------------------------------------------------------------------

Keyboard &Keyboard::instance() {
  ZoneScoped;  // NOLINT
  static Keyboard theInstance;
  return theInstance;
}

Keyboard::Keyboard() {
  ZoneScoped;  // NOLINT
  m_lastChar = 0;
  m_pCurrKeyStates = m_keyStates[0];
  m_pPrevKeyStates = m_keyStates[1];

  memset(m_pCurrKeyStates, 0, 256);
  memset(m_pPrevKeyStates, 0, 256);
}

Keyboard::~Keyboard() {}

// void Keyboard::handleMsg([[maybe_unused]] HWND hWnd, UINT msg, WPARAM wParam, [[maybe_unused]] LPARAM lParam) {
//   ZoneScoped;  // NOLINT

//   switch(msg) {
//   case WM_CHAR: m_lastChar = static_cast<int>(wParam); break;

//   default: break;
//   }
// }

void Keyboard::update() {
  ZoneScoped;  // NOLINT
  BYTE *pTempKeyStates = m_pPrevKeyStates;

  m_pPrevKeyStates = m_pCurrKeyStates;
  m_pCurrKeyStates = pTempKeyStates;

  GetKeyboardState(reinterpret_cast<BYTE *>(m_pCurrKeyStates));
}

//-----------------------------------------------------------------------------
// Mouse.
//-----------------------------------------------------------------------------

#if !defined(WHEEL_DELTA)
#  define WHEEL_DELTA 120
#endif

#if !defined(WM_MOUSEWHEEL)
#  define WM_MOUSEWHEEL 0x020A
#endif

const float Mouse::DEFAULT_WEIGHT_MODIFIER = 0.2f;

Mouse &Mouse::instance() {
  ZoneScoped;  // NOLINT
  static Mouse theInstance;
  return theInstance;
}

Mouse::Mouse() = default;

Mouse::~Mouse() { detach(); }

bool Mouse::attach(SDL_Window *window) {
  ZoneScoped;  // NOLINT
  if(nullptr == window) {
    return false;
  }

  m_window = window;

  if(!m_cursorVisible) {
    // hideCursor(true);
  }

  m_filtered[0] = 0.0f;
  m_filtered[1] = 0.0f;
  m_pCurrButtonStates = m_buttonStates[0];
  m_pPrevButtonStates = m_buttonStates[1];

  memset(m_history, 0, sizeof(m_history));
  memset(m_buttonStates, 0, sizeof(m_buttonStates));

  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  m_ptWindowCenterPos.x = width / 2;
  m_ptWindowCenterPos.y = height / 2;

  return true;
}

void Mouse::detach() {
  ZoneScoped;  // NOLINT
  if(!m_cursorVisible) {
    // hideCursor(false);

    // Save the cursor visibility state in case attach() is called later.
    m_cursorVisible = false;
  }

  m_window = nullptr;
}

void Mouse::performMouseFiltering(float x, float y) {
  ZoneScoped;  // NOLINT
  // Filter the relative mouse movement based on a weighted sum of the mouse
  // movement from previous frames to ensure that the mouse movement this
  // frame is smooth.
  //
  // For further details see:
  //  Nettle, Paul "Smooth Mouse Filtering", flipCode's Ask Midnight column.
  //  http://www.flipcode.com/cgi-bin/fcarticles.cgi?show=64462

  // Newer mouse entries towards front and older mouse entries towards end.
  for(int i = m_historyBufferSize - 1; i > 0; --i) {
    m_history[i * 2] = m_history[(i - 1) * 2];
    m_history[i * 2 + 1] = m_history[(i - 1) * 2 + 1];
  }

  // Store current mouse entry at front of array.
  m_history[0] = x;
  m_history[1] = y;

  float averageX = 0.0f;
  float averageY = 0.0f;
  float averageTotal = 0.0f;
  float currentWeight = 1.0f;

  // Filter the mouse.
  for(int i = 0; i < m_historyBufferSize; ++i) {
    averageX += m_history[i * 2] * currentWeight;
    averageY += m_history[i * 2 + 1] * currentWeight;
    averageTotal += 1.0f * currentWeight;
    currentWeight *= m_weightModifier;
  }

  m_filtered[0] = averageX / averageTotal;
  m_filtered[1] = averageY / averageTotal;
}

void Mouse::handleMsg(int delta) { m_wheelDelta += delta; }

// void Mouse::hideCursor(bool hide) {
//   ZoneScoped;  // NOLINT
//   if(hide) {
//     m_cursorVisible = false;

//     while(ShowCursor(FALSE) >= 0)
//       ;  // do nothing
//   } else {
//     m_cursorVisible = true;

//     while(ShowCursor(TRUE) < 0)
//       ;  // do nothing
//   }
// }

void Mouse::moveTo(uint32_t x, uint32_t y) {
  ZoneScoped;  // NOLINT

  SDL_WarpMouseInWindow(m_window, x, y);

  m_ptCurrentPos.x = static_cast<long>(x);
  m_ptCurrentPos.y = static_cast<long>(y);
}

void Mouse::moveToWindowCenter() {
  ZoneScoped;  // NOLINT

  if(m_ptWindowCenterPos.x != 0 && m_ptWindowCenterPos.y != 0)
    moveTo(static_cast<uint32_t>(m_ptWindowCenterPos.x), static_cast<uint32_t>(m_ptWindowCenterPos.y));
  else
    m_moveToWindowCenterPending = true;
}

void Mouse::setWeightModifier(float weightModifier) {
  ZoneScoped;  // NOLINT
  m_weightModifier = weightModifier;
}

void Mouse::smoothMouse(bool smooth) {
  ZoneScoped;  // NOLINT
  m_enableFiltering = smooth;
}

void Mouse::update() {
  ZoneScoped;  // NOLINT
  // Update mouse buttons.

  bool *pTempMouseStates = m_pPrevButtonStates;

  m_pPrevButtonStates = m_pCurrButtonStates;
  m_pCurrButtonStates = pTempMouseStates;

  int mouseX = 0, mouseY = 0;
  const auto mouseStatus = SDL_GetMouseState(&mouseX, &mouseY);
  m_pCurrButtonStates[0] = mouseStatus & SDL_BUTTON_LMASK;
  m_pCurrButtonStates[1] = mouseStatus & SDL_BUTTON_RMASK;
  m_pCurrButtonStates[2] = mouseStatus & SDL_BUTTON_MMASK;

  // Update mouse scroll wheel.

  m_mouseWheel = static_cast<float>(m_wheelDelta - m_prevWheelDelta) / static_cast<float>(WHEEL_DELTA);
  m_prevWheelDelta = m_wheelDelta;

  // Calculate the center position of the window the mouse is attached to.
  // Do this once every update in case the window has changed position or
  // size.

  int width;
  int height;
  SDL_GetWindowSize(m_window, &width, &height);
  m_ptWindowCenterPos = {width / 2, height / 2};

  if(m_moveToWindowCenterPending) {
    m_moveToWindowCenterPending = false;
    moveToWindowCenter();
  }

  // Update mouse position.
  m_ptCurrentPos = {mouseX, mouseY};
  m_ptDistFromWindowCenter = {static_cast<float>(m_ptCurrentPos.x - m_ptWindowCenterPos.x),
                              static_cast<float>(m_ptWindowCenterPos.y - m_ptCurrentPos.y)};
  // Fix SDL_GetMouseState fluctuation.
  m_ptDistFromWindowCenter.x = glm::abs(static_cast<int>(m_ptDistFromWindowCenter.x)) == 1 ? 0 : m_ptDistFromWindowCenter.x;
  m_ptDistFromWindowCenter.y = glm::abs(static_cast<int>(m_ptDistFromWindowCenter.y)) == 1 ? 0 : m_ptDistFromWindowCenter.y;

  if(m_enableFiltering) {
    performMouseFiltering(m_ptDistFromWindowCenter.x, m_ptDistFromWindowCenter.y);

    m_ptDistFromWindowCenter = {m_filtered[0], m_filtered[1]};
  }
}