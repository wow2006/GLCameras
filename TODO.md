General
-------

- [x] Remove VC++ files.
- [x] Adding a root `CMakeLists.txt`.
- [x] Adding clang-format.
- [x] Adding clang-tidy.
- [x] Adding options.cmake.
- [x] Adding download_file_.cmake.
- [x] Adding FindImgui.cmake.
- [x] Adding Findstb.cmake.
- [x] Adding Findtracy.cmake.
- [ ] Replace imgui, stb and tracy with vcpkg.json.
- [ ] Force vcpkg versions.

GLCamera1
---------

- [x] Adding `GLCamera1` to cmake.
- [x] Remove jpeg from git.
- [x] Download jpeg from google drive.
- [x] Create precompiled header.
- [x] Replace `mathlib` with glm. `NOTE: x,y-axis is inversed`
- [x] Adding OpenGL Debug.
- [x] Replace `bitmap` with stb.
- [x] Remove Multisample.
- [x] Remove Multitexture.
- [x] Adding Support for glbinding.
- [x] Port OpenGL 1.0 to OpenGL 4.6.
- [x] Remove gl_font.
- [x] Adding Imgui support.
- [x] Replace old font with Imgui background drawing.
- [x] Use DSA for OpenGL Functions.
- [x] Replace Default Uniform buffer with UBO.
- [ ] Port win32 to SDL2.
  - [ ] Missing ToggleFullscreen.
  - [ ] Missing hasFocus.
  - [ ] Missing Log function.
  - [ ] Missing `{Keyboard,Mouse}::handleMsg` in Input header.
  - [ ] Initial camera position is off.
  - [ ] Missing `HideMouse`.

GLCamera2
---------

- [x] Adding `GLCamera2` to cmake.
- [x] Remove jpeg from git.
- [x] Download jpeg from google drive.
- [x] Create precompiled header.
- [x] Replace `mathlib` with glm.
- [x] Adding OpenGL Debug.
- [x] Replace `bitmap` with stb.
- [x] Remove Multisample.
- [x] Remove Multitexture.
- [x] Adding Support for glbinding.
- [x] Port OpenGL 1.0 to OpenGL 4.6.
- [x] Remove gl_font.
- [x] Adding Imgui support.
- [x] Replace old font with Imgui background drawing.
- [x] Use DSA for OpenGL Functions.
- [x] Replace Default Uniform buffer with UBO.
- [ ] Port win32 to SDL2.
  - [ ] Missing ToggleFullscreen.
  - [ ] Missing hasFocus.
  - [ ] Missing Log function.
  - [ ] Missing `{Keyboard,Mouse}::handleMsg` in Input header.
  - [ ] Initial camera position is off.
  - [ ] Missing `HideMouse`.

GLCamera3
---------

- [x] Adding `GLCamera3` to cmake.
- [x] Delete legacy files (mathlib, bitmap, gl_font, input, GL_ARB_multitexture, WGL_ARB_multisample).
- [x] Replace `mathlib` with glm. `NOTE: quaternion multiply order is reversed vs mathlib`
- [x] Replace `bitmap` with stb.
- [x] Replace `gl_font` with ImGui.
- [x] Remove Multisample (WGL_ARB_multisample → SDL2 attributes).
- [x] Remove Multitexture (GL_ARB_multitexture → core GL).
- [x] Adding Support for glbinding.
- [x] Port OpenGL 1.0 to OpenGL 4.6.
- [x] Use DSA for OpenGL Functions.
- [x] Replace Default Uniform buffer with UBO.
- [x] Adding Imgui support.
- [x] Port win32 to SDL2.
- [x] Port camera with 4 behaviors (first person, spectator, flight, orbit).
- [x] Port model rendering (Blinn-Phong shader, per-mesh materials).
- [x] Provide bigship1.obj model file.
- [ ] Missing ToggleFullscreen.
- [ ] Missing `{Keyboard,Mouse}::handleMsg` in Input header.

GLThirdPersonCamera1
--------------------

- [x] Adding `GLThirdPersonCamera1` to cmake.
- [x] Delete legacy files (mathlib, bitmap, gl_font, input, GL_ARB_multitexture, WGL_ARB_multisample).
- [x] Replace `mathlib` with glm. `NOTE: quaternion multiply order is reversed vs mathlib`
- [x] Replace `bitmap` with stb.
- [x] Replace `gl_font` with ImGui.
- [x] Remove Multisample.
- [x] Remove Multitexture.
- [x] Adding Support for glbinding.
- [x] Port OpenGL 1.0 to OpenGL 4.6.
- [x] Use DSA for OpenGL Functions.
- [x] Replace Default Uniform buffer with UBO.
- [x] Adding Imgui support.
- [x] Port win32 to SDL2.
- [x] Port `ThirdPersonCamera` (offset-vector chase camera).
- [x] Port `Entity3D` (the rolling ball).
- [x] Replace `gluSphere` with a generated UV sphere mesh (poles on z, as GLU had them).
- [x] Add to the CI clang-format sweep.
- [x] Floor winding. `NOTE: the {3,1,0, 3,2,1} indices shared with the other demos are`
      `back-facing from above; they only survive there because GLCamera1/2 do not cull.`
- [ ] Missing ToggleFullscreen.
- [ ] Missing `{Keyboard,Mouse}::handleMsg` in Input header.

GLThirdPersonCamera2
--------------------

- [x] Adding `GLThirdPersonCamera2` to cmake.
- [x] Delete legacy files (mathlib, bitmap, gl_font, input, GL_ARB_multitexture, WGL_ARB_multisample).
- [x] Replace `mathlib` with glm. `NOTE: quaternion multiply order is reversed vs mathlib`
- [x] Replace `bitmap` with stb.
- [x] Replace `gl_font` with ImGui.
- [x] Remove Multisample.
- [x] Remove Multitexture.
- [x] Adding Support for glbinding.
- [x] Port OpenGL 1.0 to OpenGL 4.6.
- [x] Use DSA for OpenGL Functions.
- [x] Replace Default Uniform buffer with UBO.
- [x] Adding Imgui support.
- [x] Port win32 to SDL2.
- [x] Port `ThirdPersonCamera` with the critically damped spring system.
- [x] Share `Entity3D` with GLThirdPersonCamera1 via `utilities`.
- [x] Replace `gluSphere` with a generated UV sphere mesh.
- [x] Add to the CI clang-format sweep.
- [ ] Missing ToggleFullscreen.
- [ ] Missing `{Keyboard,Mouse}::handleMsg` in Input header.

OrbitCamera
-----------

Trackball
---------
