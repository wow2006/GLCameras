// Internal
#include "shaders.hpp"

inline constexpr auto InvalidShader  = -1;
inline constexpr auto InvalidProgram = -1;
inline constexpr auto MessageLength = 512;

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
#ifdef _CONSOLE
    fmt::print(fg(fmt::color::red), "ERROR: {}\n", message.data());
#else
    MessageBoxA(NULL, message.data(), "Error", MB_OK | MB_ICONEXCLAMATION);
#endif
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
#ifdef _CONSOLE
    fmt::print(fg(fmt::color::red), "ERROR: {}\n", message.data());
#else
    MessageBoxA(NULL, message.data(), "Error", MB_OK | MB_ICONEXCLAMATION);
#endif
    return InvalidProgram;
  }
  return static_cast<GLint>(programID);
}

}  // namespace Shaders

