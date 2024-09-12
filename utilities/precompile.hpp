// STL
#include <cmath>
#include <array>
#include <string>
#include <thread>
#include <vector>
#include <cstdarg>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>
// fmt
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/printf.h>
// glbinding
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
using namespace gl;
// Imgui
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
// OS
#if defined(_WIN32) && defined(_DEBUG)
#  include <crtdbg.h>
#endif
// glm
#include <glm/glm.hpp>
#include <glm/gtx/type_trait.hpp>
#include <glm/gtc/type_ptr.hpp>
