//-----------------------------------------------------------------------------
// Copyright (c) 2008 dhpoware. All Rights Reserved.
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

#include <cassert>
#include <SDL2/SDL_video.h>
#include "GL_ARB_multitexture.h"

void glActiveTextureARB(GLenum texture) {
  typedef void(APIENTRY * PFNGLACTIVETEXTUREARBPROC)(GLenum texture);
  static PFNGLACTIVETEXTUREARBPROC pfnActiveTextureARB =
    static_cast<PFNGLACTIVETEXTUREARBPROC>(SDL_GL_GetProcAddress("glActiveTextureARB"));
  assert(nullptr != pfnActiveTextureARB);
  pfnActiveTextureARB(texture);
}

void glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t) {
  typedef void(APIENTRY * PFNGLMULTITEXCOORD2FARBPROC)(GLenum target, GLfloat s, GLfloat t);
  static PFNGLMULTITEXCOORD2FARBPROC pfnMultiTexCoord2fARB =
    static_cast<PFNGLMULTITEXCOORD2FARBPROC>(SDL_GL_GetProcAddress("glMultiTexCoord2fARB"));
  assert(nullptr != pfnMultiTexCoord2fARB);
  pfnMultiTexCoord2fARB(target, s, t);
}