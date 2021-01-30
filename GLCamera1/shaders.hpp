#pragma once

namespace Shaders {

GLuint createShader(GLenum type, const char *shaderSource);

GLuint createProgram(GLuint vertexShaderID, GLuint fragmentShaderID);

}  // namespace Shaders

