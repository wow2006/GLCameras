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
// STL
#include <algorithm>
// fmt
#include <fmt/format.h>
#include <fmt/color.h>
// SDL2
#include <SDL2/SDL.h>

//-----------------------------------------------------------------------------
// Keyboard.
//-----------------------------------------------------------------------------

Keyboard &Keyboard::instance() {
  static Keyboard theInstance;
  return theInstance;
}

Keyboard::Keyboard() {
  std::fill_n(m_pCurrKeyStates, SDL_NUM_SCANCODES, 0);
  std::fill_n(m_pPrevKeyStates, SDL_NUM_SCANCODES, 0);
}

void Keyboard::update() {
  std::swap(m_pCurrKeyStates, m_pPrevKeyStates);

  const auto *keyStatus = SDL_GetKeyboardState(nullptr);
  std::copy_n(keyStatus, SDL_NUM_SCANCODES, m_pCurrKeyStates);
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

const float Mouse::DEFAULT_WEIGHT_MODIFIER = 0.2F;

Mouse &Mouse::instance() {
  static Mouse theInstance;
  return theInstance;
}

Mouse::Mouse() = default;

Mouse::~Mouse() { detach(); }

bool Mouse::attach(SDL_Window *window) {
  if(nullptr == window) {
    return false;
  }

  m_window = window;

  if(!m_cursorVisible) {
    hideCursor(true);
  }

  m_filtered[0] = 0.0F;
  m_filtered[1] = 0.0F;
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
  if(!m_cursorVisible) {
    hideCursor(false);

    // Save the cursor visibility state in case attach() is called later.
    m_cursorVisible = false;
  }

  m_window = nullptr;
}

void Mouse::performMouseFiltering(glm::vec2 pos) {
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
  m_history[0] = pos.x;
  m_history[1] = pos.y;

  glm::vec2 average = {};
  float averageTotal = 0.0F;
  float currentWeight = 1.0F;

  // Filter the mouse.
  for(int i = 0; i < m_historyBufferSize; ++i) {
    average += glm::vec2{m_history[i * 2] * currentWeight, m_history[i * 2 + 1] * currentWeight};
    averageTotal += 1.0F * currentWeight;
    currentWeight *= m_weightModifier;
  }

  m_filtered[0] = average.x / averageTotal;
  m_filtered[1] = average.y / averageTotal;
}

void Mouse::handleMsg(int delta) { m_wheelDelta += delta; }

void Mouse::hideCursor(bool hide) {
  if(hide) {
    m_cursorVisible = false;
    if(SDL_DISABLE != SDL_ShowCursor(SDL_DISABLE)) {
      fmt::print(stderr, fg(fmt::color::red), "Failed to hide cursor \"{}\"\n", SDL_GetError());
    }
  } else {
    m_cursorVisible = true;
    if(SDL_ENABLE != SDL_ShowCursor(SDL_ENABLE)) {
      fmt::print(stderr, fg(fmt::color::red), "Failed to show cursor \"{}\"\n", SDL_GetError());
    }
  }
}

void Mouse::moveTo(glm::ivec2 pos) {

  SDL_WarpMouseInWindow(m_window, pos.x, pos.y);

  m_ptCurrentPos = pos;
}

void Mouse::moveToWindowCenter() {

  if(m_ptWindowCenterPos.x != 0 && m_ptWindowCenterPos.y != 0) {
    moveTo(m_ptWindowCenterPos);
  } else {
    m_moveToWindowCenterPending = true;
  }
}

void Mouse::setWeightModifier(float weightModifier) { m_weightModifier = weightModifier; }

void Mouse::smoothMouse(bool smooth) { m_enableFiltering = smooth; }

void Mouse::update() {
  // Update mouse buttons.
  std::swap(m_pCurrButtonStates, m_pPrevButtonStates);

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

  glm::ivec2 windowRes{};
  SDL_GetWindowSize(m_window, &windowRes.x, &windowRes.y);
  m_ptWindowCenterPos = windowRes / 2;

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
    performMouseFiltering(m_ptDistFromWindowCenter);

    m_ptDistFromWindowCenter = {m_filtered[0], m_filtered[1]};
  }
}
