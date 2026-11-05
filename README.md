# The Legend of Spyro: Dawn of the Dragon — PC port

A native Windows version of the Xbox 360 game, made by static recompilation
with the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk): the game's
PowerPC code is translated to C++ ahead of time and runs directly, with the
SDK providing the Xbox 360 kernel, GPU (Direct3D 12 or Vulkan) and audio.

**This repository contains none of the game.** You need your own copy of The
Legend of Spyro: Dawn of the Dragon for the Xbox 360 (USA/Europe disc).
Setup copies the game's files from an Xbox 360 disc image (.iso, full or
trimmed) or from a folder of extracted files, and checks the version first.

> **Early release (0.1.0).** It boots and plays, mostly at 60 FPS on a
> mid-range PC (about 55 on average in the first play session, with some
> hitches), but has had little play testing. Expect bugs; reports are
> welcome in Issues (tick "Full log" in the launcher and attach
> `spyro_dotd.log`).

## Playing

Download `spyro_dotd-PC-<version>-Setup.exe` from
[Releases](../../releases), run it, and point it at your copy of the game.
Windows 10 or 11 (64-bit) with a Direct3D 12 or Vulkan graphics card.

- Any Xbox-style controller. The keyboard can play as a controller too, with
  the mouse turning the camera; keys are set in `spyro_dotd.toml`.
- The launcher sets windowed or fullscreen, window size, render sharpness
  (1x–3x of 1280×720), Direct3D 12 or Vulkan, and VSync.
- Saves are kept in `Documents\spyro_dotd`.
- Steam Deck / ROG Ally under Proton: choose Vulkan in the launcher.

See [launcher/installer/README.template.txt](launcher/installer/README.template.txt)
for the player notes that ship with Setup.

## Building

Needs Visual Studio 2022 (x64 tools and Windows SDK), LLVM/Clang 20,
CMake 3.25+, Ninja, Python 3, and your own copy of the game.

1. Build and install the SDK branch this port uses (it carries fixes the
   port needs; see its commits):
   `git clone --recursive -b spyro-dotd-0.1.0 https://github.com/Starkka15/rexglue-sdk`
   and follow its README to build and install it.
2. Put the game's files in `game/files/` (`default.xex` at the top), e.g.
   `python tools/xiso_x.py <your disc.iso> game/files`.
3. Generate the code from your `default.xex`:
   `rexglue codegen spyro_dotd_manifest.toml`
   (writes `generated/`; `config/` holds the extra function entry points the
   analyzer misses).
4. `cmake --preset win-amd64-relwithdebinfo -Drexglue_DIR=<sdk install>/lib/cmake/rexglue`
   then `cmake --build --preset win-amd64-relwithdebinfo`, from a shell with
   the VS x64 environment and Clang on PATH (`tools/rexenv.cmd` does this; edit
   its paths). Copy the SDK's `*rd.dll` files next to `spyro_dotd.exe`.
5. Optional: `launcher\package.ps1 -Sdk <sdk checkout>` builds the launcher and,
   with Inno Setup 6, the Setup program.

The generated code is the game's code translated, so it is never committed:
`generated/` and `game/` stay local. The scripts in `tools/` are the
development tools used for the port (profiling, frame analysis, finding
missed functions); several have local paths to adjust.

## Credits and licenses

The Legend of Spyro: Dawn of the Dragon was developed by Étranges Libellules
and published by Sierra Entertainment / Activision; Spyro is a trademark of
its owner. This port is not affiliated with or endorsed by any of them.

The port's own code is GPL-3.0 (see LICENSE). The ReXGlue SDK is BSD-3-Clause
and derives from the Xenia project; it bundles third-party libraries under
their own licenses (MIT, BSD, zlib, LGPL for FFmpeg and libmspack), all listed
in the `LICENSES.txt` that ships with Setup.
