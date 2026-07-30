# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> Note: a `CLAUDE.md` also exists in the parent directory (`../CLAUDE.md`) describing **Sourcetrail**, an unrelated project. It gets loaded automatically because it is an ancestor path — ignore it when working here.

## Project Overview

GLCameras collects the [dhpoware](http://www.dhpoware.com/) OpenGL camera demos into one CMake project, as a vehicle for understanding virtual camera implementations. The active work is *modernizing* the originals: fixed-function OpenGL 1.x → OpenGL 4.6 core with DSA, Win32 → SDL2, hand-rolled `mathlib` → glm, `bitmap` → stb_image, `gl_font` → ImGui, GLEW → glbinding.

`TODO.md` is the authoritative per-demo porting checklist and `CHANGELOG.md` the history — consult `TODO.md` before starting work on a demo to see what is already done and what is known-broken.

## Build

Requires **vcpkg** (with `VCPKG_ROOT` set), Ninja, and a C++17 compiler. ImGui is a **git submodule** — clone with `--recurse-submodules` or run `git submodule update --init`.

```bash
cmake --preset=vc_release && cmake --build ../build/Release
```

Presets: `vc_release` / `vc_debug` (Windows, MSVC), `gnu_release` / `gnu_debug`, `clang_release` (Linux). All inherit the vcpkg toolchain and Ninja, and are guarded by a `hostSystemName` condition so the wrong-platform presets simply won't configure.

Two gotchas:
- `binaryDir` is `${sourceDir}/../build/<Config>` — the build tree lands **beside** the repo, not inside it.
- The `debug` preset sets `CMAKE_BUILD_TYPE=RelWithDebInfo`, not `Debug`.

Use `CMakeUserPresets.json` for local, uncommitted preset tweaks.

## Running

Textures are staged next to the binary at **configure** time, so run each executable from its own build subdirectory or `stbi_load` won't find them. Two mechanisms are in play: `GLCamera1` and `GLCamera2` fetch their floor jpgs with `download_file()` (`cmake/download_file.cmake`) from Google Drive with MD5 checks, cached in the source dir; `GLCamera3` and the two third person demos keep their assets in the repo and just `file(COPY ...)` them.

To get OpenGL debug output (`glDebugMessageCallback` + `GL_DEBUG_OUTPUT_SYNCHRONOUS`), uncomment `OPENGL_DEBUG` in the demo's `target_compile_definitions`. Note `GLCamera1/CMakeLists.txt:24` has it commented out as `OPENG_DEBUG` (misspelled) while the code checks `OPENGL_DEBUG`.

## Formatting

There is no test suite. CI (`.github/workflows/build.yaml`) gates the build job on two format checks, so run these before pushing:

```bash
find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -not -path "*/thirdparty/*" -not -path "*/GLCamera3/*" -not -path "*/OrbitCamera/*" -not -path "*/Trackball/*" -exec clang-format -n --Werror {} \;
```

```bash
find . -type f -name "CMakeLists.txt" -exec cmake-format --check {} \;
```

CI pins **clang-format-15** and `cmake-format` from `cmakelang` (no `.cmake-format.yaml`, so defaults apply). `.clang-format` is notably a 130-column limit, 2-space indent, `SpaceBeforeParens: Never` (so `if(x)`, `while(x)`), `IndentPPDirectives: AfterHash`, and `SortIncludes: false` — include order is hand-curated, don't let a tool reshuffle it.

Two traps when checking formatting locally:

- **Match the pinned version.** clang-format's output changes between releases; v22 (the one bundled with Visual Studio) reformats files that v15 considers clean, so checking with it produces false failures. `pip install clang-format==15.0.7` gets the exact CI binary.
- **The command in `build.yaml` is not the one above.** It omits the `\( … \)` grouping, and `find` binds `-and` tighter than `-or`, so the `-exec` attaches only to the `*.hpp` branch — CI never actually checks `.cpp` or `.h` files. The grouped command above is the intended check and is stricter than CI; several `.cpp` files under `GLCamera1`/`GLCamera2` fail it today.
- `cmake-format --check` compares against LF output, so on a CRLF checkout **every** file reports as unformatted. Diff the formatted output with line endings normalised instead.

`.clang-tidy` enables a broad set (`cppcoreguidelines-*`, `modernize-*`, `readability-*`, `performance-*`) but is not wired into CMake or CI; treat it as advisory.

## Driving a demo to verify a change

The demos have no tests, so the only real check is running one and looking at it. If you script that with `keybd_event`, **pass a real scan code** (`MapVirtualKey(vk, MAPVK_VK_TO_VSC)`), not `0`. SDL keys off scan codes, so `bScan = 0` silently delivers nothing for letter and space keys, while extended keys like the arrows still work — which looks exactly like broken `keyPressed` edge detection in the app. `Keyboard::keyPressed` is fine; it fires on precisely one frame.

Screenshots need `SetProcessDPIAware()` in the capturing process, or `GetClientRect`/`ClientToScreen` return logical pixels while `CopyFromScreen` works in physical ones and the grab is offset and cropped.

## Architecture

### What is actually built

The root `CMakeLists.txt` builds `utilities`, `GLCamera1`, `GLCamera2`, `GLCamera3`, `GLThirdPersonCamera1`, and `GLThirdPersonCamera2`. `OrbitCamera` and `Trackball` are **commented out** — they are still unported original sources (Win32 `WinMain`, `WGL_ARB_multisample`, `gl_font`, `bitmap`, `.rc` files) and are deliberately excluded from the clang-format sweep. Adding one to the build means porting it, not just uncommenting a line.

`GLCamera3` is built but still excluded from the clang-format sweep: its sources (`model_obj.cpp` especially) carry a lot of unreformatted original dhpoware code.

### `utilities` (`camera::utilities`)

The shared static lib, and the place ported code should converge on:

- `entity3d.hpp/cpp` — `Entity3D`, the position/orientation carrier the two third person demos drive their ball with. Shared because both demos had byte-for-byte equivalent copies; the per-demo camera classes are *not* shared, because those genuinely differ.
- `input.hpp/cpp` — SDL2-backed `Keyboard` and `Mouse` singletons (`::instance()`), with double-buffered state and `keyPressed`/`buttonPressed` edge detection. `Mouse` also does history-buffer smoothing, window-center recentering, and cursor hiding. Per `TODO.md`, `{Keyboard,Mouse}::handleMsg` is still an incomplete part of the Win32→SDL2 port.
- `shaders.hpp/cpp` — `Shaders::createShader` / `Shaders::createProgram`.
- `precompile.hpp` — the PCH every demo sets via `target_precompile_headers`. It pulls in STL, fmt, glbinding, ImGui, and glm, and issues a global **`using namespace gl;`**. That is why demo code calls `glCreateBuffers`, `GL_TRIANGLES`, etc. unqualified despite glbinding putting everything in `namespace gl` — new translation units need this header (or their own `using namespace gl`) to compile.

### Demo structure

Each demo is one large `main.cpp` in the original dhpoware idiom: file-scope `g_*` globals, a block of forward declarations, then definitions. `main()` owns SDL init, window creation, and the event loop; the loop body is `UpdateFrame(GetElapsedTimeInSeconds())` → `RenderFrame()` → `SDL_GL_SwapWindow`. `Init()`/`InitGL()`/`InitApp()` and `Cleanup()`/`CleanupApp()` bracket it. Follow the surrounding style when editing rather than restructuring.

Rendering is modern-GL: GLSL 4.60 shaders as `constexpr std::string_view` raw literals inside `createProgram()`, VAO/VBO/EBO created with `glCreateBuffers`/`glNamedBufferStorage` (DSA, no bind-to-edit), and the view/projection matrices delivered through a `std140` UBO at binding point 0 rather than default uniforms. ImGui (SDL2 + OpenGL3 backends) replaces the original bitmap-font HUD.

### The cameras

- **GLCamera1** — vector-based 6DoF camera. First person (5DoF, movement parallel to the x-z plane) and flight (6DoF) via `Camera::CameraBehavior`.
- **GLCamera2** — the same two behaviors, quaternion-based.
- **GLCamera3** — quaternion-based with four behaviors (first person, spectator, flight, orbit), rendering an OBJ model with Blinn-Phong shading.
- **GLThirdPersonCamera1** — chase camera. The user drives a rolling ball (`Entity3D`) and the camera orbits a fixed offset vector around it, snapping to position each frame.
- **GLThirdPersonCamera2** — the same chase camera with a **critically damped spring** between the camera and the ball, so it trails and settles instead of snapping. `setSpringConstant()` keeps the damping ratio at one by deriving `dampingConstant = 2*sqrt(springConstant)`. SPACE toggles the spring, `+`/`-` change its stiffness.

### Porting mathlib to glm

Every port so far has hit the same three things, all of them silent if you get them wrong:

- **Quaternion product order is reversed.** mathlib's `operator*=` composes rotations *left to right* (the standard right-to-left version is sitting commented out inside it), so every mathlib `a * b` becomes `b * a` in glm. A useful check: the offset-vector triple product `conj(q)*v*q` in mathlib becomes `q*v*conj(q)` in glm, which is exactly what glm's quat-vector product already does.
- **Matrix element assignments carry over verbatim.** mathlib is row-vector and stores `m[row][col]`; glm is column-vector and stores `m[col][row]`. The two land on identical memory, so copying index expressions unchanged from the original gives the correct glm matrix, and `Quaternion::toMatrix4()` maps straight onto `glm::mat4_cast`.
- **glm's x/y axes are inverted relative to `mathlib`** — see the note in `TODO.md` for GLCamera1.

Also watch for `glm::normalize` on a possibly-zero vector: mathlib's `normalize()` guarded that case, glm returns NaNs.

### Dependencies

vcpkg (`vcpkg.json`, pinned via `builtin-baseline`) provides fmt, glbinding, glm, stb, and SDL2. ImGui is the vendored submodule under `thirdparty/imgui`, compiled by `cmake/Modules/FindImgui.cmake` into `Imgui::core`, `Imgui::SDL2`, `Imgui::OpenGL`, and (Windows-only) `Imgui::Win32` — warnings suppressed and headers marked `SYSTEM`. `cmake/options.cmake` defines the `options::options` interface target holding the shared strict warning set (`-Wconversion`, `-Wold-style-cast`, `-Wshadow`, MSVC `/W4 /permissive-`, …); link demo targets against it.
