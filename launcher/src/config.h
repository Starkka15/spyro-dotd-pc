/* <game>.toml beside the game: the ReXGlue SDK reads it at startup (one flat
 * "cvar = value" per line; a [section] would prefix the key names, so there
 * are none). The launcher edits it line by line and keeps everything else. */
#ifndef REX_LAUNCHER_CONFIG_H
#define REX_LAUNCHER_CONFIG_H

typedef struct {
    int fullscreen;
    int window_width, window_height;
    int resolution_scale;      /* 1-3 */
    int vsync;
    int backend;               /* 0 any, 1 d3d12, 2 vulkan */
    int mnk_mode, mnk_mouse;
    double mnk_sensitivity;
    int full_log;
} GameConfig;

extern const GameConfig CONFIG_DEFAULTS;

/* Creates the file with commented defaults when it is missing. */
void config_ensure(void);
void config_load(GameConfig *c);
void config_save(const GameConfig *c);
/* Full path of the toml file. */
const wchar_t *config_path(void);

#endif
