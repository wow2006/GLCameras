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

- [x] Adding `OrbitCamera` to cmake.
- [x] Delete legacy files (mathlib, Tga, ObjModel, BitmapFont, glExtension, Win32
      MVC: Window/DialogWindow/Controller*/View*/procedure/Log/wcharUtil, `.rc`).
- [x] Replace `mathlib` with glm. `NOTE: mathlib's Matrix4 m[i] is glm's`
      `matrix[i / 4][i % 4], so every index expression carries over verbatim.`
- [x] Replace `ObjModel` with the shared `ModelOBJ` loader in `utilities`.
- [x] Adding Support for glbinding.
- [x] Port OpenGL 1.0 to OpenGL 4.6 (Blinn-Phong replaces the fixed function
      light, the `gl_ARB_shader_objects`/`ARB_vertex_buffer_object` paths and
      the `glExtension` probing are gone).
- [x] Use DSA for OpenGL Functions.
- [x] Replace Default Uniform buffer with UBO.
- [x] Adding Imgui support.
- [x] Replace `BitmapFont` with ImGui.
- [x] Port the two child GL windows to two `glViewport`/`glScissor` viewports.
- [x] Port the modeless control dialog to an ImGui panel (angles, position,
      target, FOV, grid/FOV toggles, reset, live matrix + quaternion readout).
- [x] Port win32 to SDL2.
- [x] Add to the CI clang-format sweep.
- [x] Provide `data/debugger_small_5k.obj` and `data/camera.obj`.
- [ ] Missing ToggleFullscreen.
- [ ] Missing `{Keyboard,Mouse}::handleMsg` in Input header.

Two deliberate deviations from the original, both noted in the source:

- `setRotation(angle)` handed raw degrees to `Quaternion::getQuaternion()`,
  which wanted half angles in radians, so the quaternion the dialog printed
  after a slider move was meaningless. It now uses the same conversion
  `lookAt()` already did. The quaternion is read-only for the demo, so nothing
  else moves.
- The FOV slider stays live when the FOV checkbox is cleared. The checkbox only
  hides the cone, but the same value also drives the Point of View projection.

Trackball
---------

- [x] Adding `Trackball` to cmake.
- [x] Delete legacy files (mathlib, animUtils, Primitives, GLUT).
- [x] Replace `mathlib` with glm. `NOTE: mathlib's quaternion product is the`
      `standard Hamilton product here, so "delta * prevQuat" keeps its order;`
      `only Quaternion(axis, angle) differs, taking a half angle where`
      `glm::angleAxis() takes the full one.`
- [x] Adding Support for glbinding.
- [x] Port OpenGL 1.0 to OpenGL 4.6.
- [x] Use DSA for OpenGL Functions.
- [x] Replace Default Uniform buffer with UBO.
- [x] Adding Imgui support.
- [x] Replace the GLUT bitmap font with ImGui.
- [x] Replace `gluSphere` with a generated UV sphere mesh (poles on z, as GLU
      had them).
- [x] Port GLUT to SDL2.
- [x] Port the `Trackball` class (ARC and PROJECT cursor-to-sphere mapping).
- [x] Add to the CI clang-format sweep.
- [x] Replace the placeholder wire object with `data/debugger_small_5k.obj`,
      the same OBJ `OrbitCamera` uses, via the shared `ModelOBJ` loader.
      `NOTE: the original drew glutWireTeapot(). GLUT is gone and the Newell`
      `patch data is not in this repository, and the debugger model is what's`
      `available; the demo falls back to a generated wire torus if the OBJ is`
      `absent, which is just as obviously asymmetric under rotation — the only`
      `job the teapot had.`
- [ ] Missing ToggleFullscreen.
- [ ] Missing `{Keyboard,Mouse}::handleMsg` in Input header.
- [ ] `glLineWidth` above 1.0 is not guaranteed in a core profile, so the mouse
      path and the axis may come out thinner than the original.

MayaCamera
----------

- [x] Adding `MayaCamera` to cmake.
- [x] `MayaCamera` class: tumble, track, dolly, frame, orthographic bookmarks,
      tumble pivot modes and view undo/redo, in glm only (no GL, no SDL).
- [x] Two viewport demo, reusing `OrbitCamera`'s scene, grid, `ModelOBJ` models
      and ImGui panel layout.
- [x] Draw the orthographic view volume as a prism when the demo camera is
      orthographic, instead of the perspective FOV cone.
- [x] `--self-test` flag running `mayaCameraSelfTest()` headless.
- [ ] Missing ToggleFullscreen (shared with the other demos).
- [ ] Tumbling over a pole flips the image, as Maya does; the observer camera in
      the left viewport is clamped to +/-89 instead.
- [ ] `setTumblePivot()` re-derives the view from the new pivot, which collapses
      an over-the-pole orientation back into |pitch| <= 90.
