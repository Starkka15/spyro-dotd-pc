/* Installing the game files from a folder, disc or Xbox 360 disc image: see
 * import.c. */
#ifndef REX_LAUNCHER_IMPORT_H
#define REX_LAUNCHER_IMPORT_H

#include <windows.h>

enum { IMPORT_OK = 0, IMPORT_FAILED = 1, IMPORT_WRONG_VERSION = 2, IMPORT_CANCELLED = 3 };

/* Copies the game from `src` (a folder, drive or image file) into `dest`,
 * with a progress window. Errors are shown to the player; `quiet` leaves out
 * the "installed" message on success. Returns an IMPORT_ code. */
int import_game(HWND parent, const wchar_t *src, const wchar_t *dest, int quiet);

/* Whether `dest` holds the game (its default.xex). */
int import_installed(const wchar_t *dest);

#endif
