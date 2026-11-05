/*
 * <game>.toml: see config.h.
 *
 * Values are written in TOML's own types (true/false, numbers, "strings"),
 * which the SDK turns into its cvars. Keys the launcher does not know --
 * key bindings, or anything a player adds by hand -- are left as they are.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "config.h"

const GameConfig CONFIG_DEFAULTS = {
    .fullscreen = 0,
    .window_width = 1280,
    .window_height = 720,
    .resolution_scale = 1,
    .vsync = 1,
    .backend = 0,
    .mnk_mode = 1,
    .mnk_mouse = 1,
    .mnk_sensitivity = 1.0,
    .full_log = 0,
};

static const char DEFAULT_FILE[] =
    "# " GAME_TITLE_A " -- settings, read by the game at startup.\r\n"
    "# The launcher edits this file; you can too. Delete it to get the defaults back.\r\n"
    "\r\n"
    "# --- Display ---\r\n"
    "# true = borderless fullscreen.\r\n"
    "fullscreen = false\r\n"
    "# Window size in windowed mode.\r\n"
    "window_width = 1280\r\n"
    "window_height = 720\r\n"
    "# Sharpness: the game draws at 1280x720 times this (1-3). Higher costs more GPU.\r\n"
    "resolution_scale = 1\r\n"
    "# true = wait for the monitor's refresh (no tearing).\r\n"
    "vsync = true\r\n"
    "# Graphics API: \"any\" (Direct3D 12 first), \"d3d12\" or \"vulkan\".\r\n"
    "gpu_backend = \"any\"\r\n"
    "\r\n"
    "# --- Keyboard and mouse ---\r\n"
    "# true = the keyboard plays as a controller, alongside any real one.\r\n"
    "mnk_mode = true\r\n"
    "# true = the mouse turns the camera (right stick).\r\n"
    "mnk_mouse = true\r\n"
    "mnk_sensitivity = 1.0\r\n"
    "# Key bindings: each is a comma-separated list of keys; Shift+, Ctrl+ and\r\n"
    "# Alt+ prefixes need that modifier held. Empty = unbound.\r\n"
    "keybind_a = \"Space,Semicolon\"\r\n"
    "keybind_b = \"Backspace,Quote\"\r\n"
    "keybind_x = \"L\"\r\n"
    "keybind_y = \"P\"\r\n"
    "keybind_left_trigger = \"Q,I\"\r\n"
    "keybind_right_trigger = \"E,O\"\r\n"
    "keybind_left_shoulder = \"1\"\r\n"
    "keybind_right_shoulder = \"3\"\r\n"
    "keybind_lstick_up = \"W\"\r\n"
    "keybind_lstick_down = \"S\"\r\n"
    "keybind_lstick_left = \"A\"\r\n"
    "keybind_lstick_right = \"D\"\r\n"
    "keybind_lstick_press = \"F\"\r\n"
    "keybind_rstick_up = \"Up\"\r\n"
    "keybind_rstick_down = \"Down\"\r\n"
    "keybind_rstick_left = \"Left\"\r\n"
    "keybind_rstick_right = \"Right\"\r\n"
    "keybind_rstick_press = \"K\"\r\n"
    "keybind_dpad_up = \"Shift+Up\"\r\n"
    "keybind_dpad_down = \"Shift+Down\"\r\n"
    "keybind_dpad_left = \"Shift+Left\"\r\n"
    "keybind_dpad_right = \"Shift+Right\"\r\n"
    "keybind_back = \"Tab,Z\"\r\n"
    "keybind_start = \"Return,X\"\r\n"
    "\r\n"
    "# --- Troubleshooting ---\r\n"
    "# The log beside the game. \"info\" writes a full log for bug reports.\r\n"
    "log_file = \"" GAME_ID ".log\"\r\n"
    "log_level = \"warn\"\r\n";

static wchar_t s_path[MAX_PATH];

const wchar_t *config_path(void)
{
    if (!s_path[0]) {
        DWORD n = GetModuleFileNameW(NULL, s_path, MAX_PATH);
        wchar_t *slash = n && n < MAX_PATH ? wcsrchr(s_path, L'\\') : NULL;
        if (slash && (size_t)(slash - s_path) + 20 + strlen(GAME_ID) < MAX_PATH)
            swprintf(slash + 1, MAX_PATH - (slash + 1 - s_path), L"%hs.toml", GAME_ID);
        else
            s_path[0] = 0;
    }
    return s_path;
}

void config_ensure(void)
{
    FILE *f;
    if (!config_path()[0])
        return;
    f = _wfopen(s_path, L"rb");
    if (f) {
        fclose(f);
        return;
    }
    f = _wfopen(s_path, L"wb");
    if (f) {
        fwrite(DEFAULT_FILE, 1, sizeof DEFAULT_FILE - 1, f);
        fclose(f);
    }
}

static void trim(char **s)
{
    char *e;
    while (**s == ' ' || **s == '\t')
        (*s)++;
    e = *s + strlen(*s);
    while (e > *s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n'))
        *--e = 0;
}

static int is_true(const char *v)
{
    return !_stricmp(v, "true") || !strcmp(v, "1");
}

static void apply(GameConfig *c, const char *k, char *v)
{
    int n;
    /* Strip quotes and a trailing # comment from simple values. */
    if (*v == '"') {
        char *q = strchr(v + 1, '"');
        v++;
        if (q)
            *q = 0;
    } else {
        char *h = strchr(v, '#');
        if (h) {
            *h = 0;
            trim(&v);
        }
    }
    n = atoi(v);
    if (!strcmp(k, "fullscreen"))
        c->fullscreen = is_true(v);
    else if (!strcmp(k, "window_width") && n >= 320 && n <= 16384)
        c->window_width = n;
    else if (!strcmp(k, "window_height") && n >= 240 && n <= 16384)
        c->window_height = n;
    else if (!strcmp(k, "resolution_scale") && n >= 1 && n <= 3)
        c->resolution_scale = n;
    else if (!strcmp(k, "vsync"))
        c->vsync = is_true(v);
    else if (!strcmp(k, "gpu_backend"))
        c->backend = !_stricmp(v, "d3d12") ? 1 : !_stricmp(v, "vulkan") ? 2 : 0;
    else if (!strcmp(k, "mnk_mode"))
        c->mnk_mode = is_true(v);
    else if (!strcmp(k, "mnk_mouse"))
        c->mnk_mouse = is_true(v);
    else if (!strcmp(k, "mnk_sensitivity") && atof(v) > 0.0)
        c->mnk_sensitivity = atof(v);
    else if (!strcmp(k, "log_level"))
        c->full_log = !_stricmp(v, "info") || !_stricmp(v, "debug") || !_stricmp(v, "trace");
}

void config_load(GameConfig *c)
{
    FILE *f;
    char line[512];

    *c = CONFIG_DEFAULTS;
    config_ensure();
    if (!config_path()[0] || !(f = _wfopen(s_path, L"rb")))
        return;
    while (fgets(line, sizeof line, f)) {
        char *k = line, *v, *eq;
        trim(&k);
        if (!*k || *k == '#' || *k == '[')
            continue;
        eq = strchr(k, '=');
        if (!eq)
            continue;
        *eq = 0;
        v = eq + 1;
        trim(&k);
        trim(&v);
        apply(c, k, v);
    }
    fclose(f);
}

/* Replaces the value on the key's line, or appends the key; everything else
 * in the file stays as it was. */
static void set_line(const char *key, const char *value)
{
    FILE *f;
    char *buf, *out, *p;
    long size = 0;
    size_t klen = strlen(key), used = 0, cap;
    int found = 0;

    f = _wfopen(s_path, L"rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);
        buf = (char *)malloc((size_t)size + 1);
        if (!buf) {
            fclose(f);
            return;
        }
        size = (long)fread(buf, 1, (size_t)size, f);
        buf[size] = 0;
        fclose(f);
    } else {
        buf = _strdup(DEFAULT_FILE);
        if (!buf)
            return;
        size = (long)strlen(buf);
    }
    cap = (size_t)size + klen + strlen(value) + 32;
    out = (char *)malloc(cap);
    if (!out) {
        free(buf);
        return;
    }
    for (p = buf; *p;) {
        char *eol = strchr(p, '\n');
        size_t len = eol ? (size_t)(eol - p) + 1 : strlen(p);
        char *k = p;
        while (k < p + len && (*k == ' ' || *k == '\t'))
            k++;
        if (!found && !strncmp(k, key, klen)) {
            char *q = k + klen;
            while (*q == ' ' || *q == '\t')
                q++;
            if (*q == '=') {
                used += (size_t)sprintf(out + used, "%s = %s\r\n", key, value);
                found = 1;
                p += len;
                continue;
            }
        }
        memcpy(out + used, p, len);
        used += len;
        p += len;
    }
    if (!found) {
        if (used && out[used - 1] != '\n')
            used += (size_t)sprintf(out + used, "\r\n");
        used += (size_t)sprintf(out + used, "%s = %s\r\n", key, value);
    }
    f = _wfopen(s_path, L"wb");
    if (f) {
        fwrite(out, 1, used, f);
        fclose(f);
    }
    free(out);
    free(buf);
}

static void set_int(const char *key, int v)
{
    char t[16];
    snprintf(t, sizeof t, "%d", v);
    set_line(key, t);
}

void config_save(const GameConfig *c)
{
    static const char *const BACKENDS[] = { "\"any\"", "\"d3d12\"", "\"vulkan\"" };
    char t[32];

    if (!config_path()[0])
        return;
    set_line("fullscreen", c->fullscreen ? "true" : "false");
    set_int("window_width", c->window_width);
    set_int("window_height", c->window_height);
    set_int("resolution_scale", c->resolution_scale);
    set_line("vsync", c->vsync ? "true" : "false");
    set_line("gpu_backend", BACKENDS[c->backend >= 0 && c->backend <= 2 ? c->backend : 0]);
    set_line("mnk_mode", c->mnk_mode ? "true" : "false");
    set_line("mnk_mouse", c->mnk_mouse ? "true" : "false");
    snprintf(t, sizeof t, "%.2f", c->mnk_sensitivity);
    set_line("mnk_sensitivity", t);
    set_line("log_level", c->full_log ? "\"info\"" : "\"warn\"");
}
