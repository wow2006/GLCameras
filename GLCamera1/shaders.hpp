#pragma once

namespace Shaders {

GLint createShader(GLenum type, const char *shaderSource);

GLint createProgram(GLint vertexShaderID, GLint fragmentShaderID);

}  // namespace Shaders

