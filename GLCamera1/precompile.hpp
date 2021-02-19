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
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
// SDL2
#include <SDL2/SDl.h>
// OS
#ifdef _WIN32
#include <windows.h>
#include <VersionHelpers.h>

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

inline double get_time() {
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double)t.QuadPart * 1000.0 / (double)f.QuadPart;
}
#else
#include <sys/time.h>
#include <sys/resource.h>

inline double get_time() {
    struct timeval t;
    struct timezone tzp;
    gettimeofday(&t, &tzp);
    return t.tv_usec * 0.002;
}
#endif
// Tracy
#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#else
#  define ZoneScoped
#  define FrameMark
#endif
// glm
#include <glm/glm.hpp>
#include <glm/gtx/type_trait.hpp>
#include <glm/gtc/type_ptr.hpp>
