// Internal
#include "shaders.hpp"

#include <fmt/printf.h>
#include <fmt/color.h>

namespace {
constexpr auto InvalidShader = -1;
constexpr auto InvalidProgram = -1;
constexpr auto MessageLength = 512;
}  // namespace

namespace Shaders {

GLint createShader(GLenum type, const char *shaderSource) {
  const GLuint shaderID = glCreateShader(type);

  glShaderSource(shaderID, 1, &shaderSource, nullptr);
  glCompileShader(shaderID);

  GLint result = 0;
  glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
  if(result != 1) {
    std::array<GLchar, MessageLength> message = {};
    glGetShaderInfoLog(shaderID, MessageLength, nullptr, message.data());
    fmt::print(fg(fmt::color::red), "ERROR: {}\n", message.data());
    return InvalidShader;
  }
  return static_cast<GLint>(shaderID);
}

GLint createProgram(GLint vertexShaderID, GLint fragmentShaderID) {
  if(vertexShaderID == InvalidShader || fragmentShaderID == InvalidShader) {
    fmt::print(fg(fmt::color::red), "ERROR: Invalid shaders\n");
    return InvalidProgram;
  }

  const auto programID = glCreateProgram();
  glAttachShader(programID, static_cast<GLuint>(vertexShaderID));
  glAttachShader(programID, static_cast<GLuint>(fragmentShaderID));
  glLinkProgram(programID);

  // Check the program
  GLint result = 0;
  glGetProgramiv(programID, GL_LINK_STATUS, &result);
  if(result != 1) {
    std::array<GLchar, MessageLength> message = {};
    glGetProgramInfoLog(programID, MessageLength, nullptr, message.data());
    fmt::print(fg(fmt::color::red), "ERROR: {}\n", message.data());
    return InvalidProgram;
  }
  return static_cast<GLint>(programID);
}

}  // namespace Shaders
