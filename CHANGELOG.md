# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Adding `MayaCamera`, an Autodesk Maya style viewport camera (tumble, track,
  dolly, frame, orthographic bookmarks and view undo/redo) with a two viewport
  demo and a `--self-test` flag.
- Adding `OrbitCamera` to cmake, ported to SDL2, OpenGL 4.6 and glm.
- Adding `Trackball` to cmake, ported to SDL2, OpenGL 4.6 and glm.
- Adding a root `CMakeLists.txt`.
- Adding clang-format.
- Adding clang-tidy.
- Adding `GLCamera1` to cmake.
- Adding options.cmake.
- Adding Findtracy.cmake.
- Adding jpeg frpm google drive.
- Adding precompiled header.
- Adding glm library.
- Adding OpenGL Debug.
- Adding Support for GL3W.

### Changed
- Replace `bitmap` with stb.
- Replace `mathlib` with glm.
- Port OpenGL 1.0 to OpenGL 4.6
- Replace OpenGL 3.3 functions with DSA.
- Share the `ModelOBJ` loader between `GLCamera3` and `OrbitCamera` via
  `utilities`.
- Replace `OrbitCamera`'s Win32 MVC shell with two GL viewports and an ImGui
  control panel.
- Replace `Trackball`'s GLUT shell with SDL2 and ImGui.

### Removed
- Remove VC++ files.
- Remove jpeg from git.
- Remove gl_font.
- Remove Multisample.
- Remove Multitexture.

## [1.0.0] - 2021-1-28
### Added
- Adding a GLCamera1 project.
- Adding a GLCamera2 project.
- Adding a GLCamera3 project.
- Adding a GLThirdPersonCamera1 project.
- Adding a GLThirdPersonCamera1 project.
- Adding a OrbitCamera project.
- Adding a Trackball project.
- Adding a Tracy submodule.
- Integrate Tracy inside GLCamera1 project.

[1.0.0]: https://github.com/wow2006/GLCameras/releases/tag/1.0.0
