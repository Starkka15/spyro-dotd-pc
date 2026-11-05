{TITLE} -- PC port {VERSION}
==========================================================

A native Windows port of {DISC}, made by statically recompiling the game's
Xbox 360 code with the ReXGlue SDK. This is an early release.

This port contains none of the game. You need your own copy: an image of
the Xbox 360 disc (.iso, full or trimmed) or its files already extracted.
Setup asks for it, or use "Install game files..." in the launcher later.
Other versions or regions of the game are refused, because the port is
made from one specific version of the program.


Playing
-------
Start "{TITLE}" from the Start menu or desktop. That opens the launcher:
set the options, then press Play. "Play (skip launcher)" starts the game
directly with the last settings.

Controllers: any Xbox-style controller works. The keyboard (and the mouse
for the camera) can play as a controller too; see "Key bindings..." in the
launcher.


Settings
--------
The launcher writes {ID}.toml beside the game. You can edit it by hand;
delete it to get the defaults back.

  Sharpness       the game draws at 1280x720 times this. Higher looks
                  sharper and costs more GPU.
  Graphics API    Direct3D 12 is the default. Vulkan works on more systems
                  (for example Steam Deck / ROG Ally under Proton).
  VSync           waits for the monitor's refresh: no tearing.


Saves
-----
Saves are kept in Documents\{ID}. Uninstalling leaves them there.


Problems
--------
Tick "Full log (for bug reports)" in the launcher and play until the
problem happens; {ID}.log beside the game then has the details.


Licenses
--------
LICENSES.txt lists the licenses of the ReXGlue SDK (derived from the Xenia
project) and the libraries it uses. The game itself belongs to its
publishers; nothing of it is included here.
