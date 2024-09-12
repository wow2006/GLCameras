#pragma once
#include <glbinding/gl/gl.h>
using namespace gl;

namespace Shaders {

GLint createShader(GLenum type, const char *shaderSource);

GLint createProgram(GLint vertexShaderID, GLint fragmentShaderID);

}  // namespace Shaders