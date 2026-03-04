#include <coreinit/filesystem.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/debug.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CFG_PATH "/vol/storage_sdcard/wiiu/abw.cfg"
#define LOG_PATH "/vol/storage_sdcard/wiiu/abw.log"

/* ---------------- TYPES ---------------- */

typedef enum {
    WAKE_OTHER = 0,
    WAKE_GAMEPAD,
    WAKE_WIIMOTE
} wake_t;

typedef struct {
    int timeout;
    int log;
    int safe_mode;

    int wiimote_enable;
    int gamepad_enable;

    char boot_default[256];
    char boot_gamepad[256];
    char boot_wiimote[256];
    char fallback[64];
} config_t;

static config_t g_cfg;

/* ---------------- LOG ---------------- */

static void log_line(const char *msg) {
    if (!g_cfg.log) return;

    FSClient client;
    FSCmdBlock block;
    FSFileHandle fh;

    FSInit();
    FSInitCmdBlock(&block);
    FSAddClient(&client, FS_ERROR_FLAG_NONE);

    if (FSOpenFile(&client, &block, LOG_PATH, "a", &fh, -1) == FS_STATUS_OK) {
        FSWriteFile(&client, &block, (uint8_t*)msg, 1, strlen(msg), fh, 0, -1);
        FSCloseFile(&client, &block, fh, -1);
    }

    FSDelClient(&client, -1);
}

/* ---------------- CONFIG ---------------- */

static void cfg_defaults() {
    memset(&g_cfg, 0, sizeof(g_cfg));

    g_cfg.timeout = 10;
    g_cfg.log = 1;
    g_cfg.safe_mode = 1;
    g_cfg.wiimote_enable = 1;
    g_cfg.gamepad_enable = 1;

    strcpy(g_cfg.boot_default,
        "/vol/storage_sdcard/wiiu/apps/homebrew_launcher/homebrew_launcher.rpx");

    strcpy(g_cfg.fallback, "wiiu_menu");
}

static void parse_line(char *l) {
    if (l[0] == '#' || strlen(l) < 3) return;

    char *eq = strchr(l, '=');
    if (!eq) return;

    *eq = 0;
    char *k = l;
    char *v = eq + 1;

    if (!strcmp(k, "timeout")) g_cfg.timeout = atoi(v);
    else if (!strcmp(k, "log")) g_cfg.log = atoi(v);
    else if (!strcmp(k, "safe_mode")) g_cfg.safe_mode = atoi(v);

    else if (!strcmp(k, "wiimote_enable")) g_cfg.wiimote_enable = atoi(v);
    else if (!strcmp(k, "gamepad_enable")) g_cfg.gamepad_enable = atoi(v);

    else if (!strcmp(k, "boot_default")) strncpy(g_cfg.boot_default, v, 255);
    else if (!strcmp(k, "boot_gamepad")) strncpy(g_cfg.boot_gamepad, v, 255);
    else if (!strcmp(k, "boot_wiimote")) strncpy(g_cfg.boot_wiimote, v, 255);

    else if (!strcmp(k, "fallback")) strncpy(g_cfg.fallback, v, 63);
}

static void load_config() {
    cfg_defaults();

    FSClient client;
    FSCmdBlock block;
    FSFileHandle fh;

    FSInit();
    FSInitCmdBlock(&block);
    FSAddClient(&client, FS_ERROR_FLAG_NONE);

    if (FSOpenFile(&client, &block, CFG_PATH, "r", &fh, -1) != FS_STATUS_OK) {
        log_line("abw: no cfg, defaults\n");
        return;
    }

    char buf[512];
    int r;

    while ((r = FSReadFile(&client, &block, (uint8_t*)buf, 1, sizeof(buf)-1, fh, 0, -1)) > 0) {
        buf[r] = 0;
        char *l = strtok(buf, "\n");
        while (l) {
            parse_line(l);
            l = strtok(NULL, "\n");
        }
    }

    FSCloseFile(&client, &block, fh, -1);
    FSDelClient(&client, -1);
}

/* ---------------- SAFE ---------------- */

static int safe_pressed_stub() {
    // real VPAD hook later
    return 0;
}

/* ---------------- WIIMOTE ---------------- */

static int wiimote_present_stub() {
    // IOS BT hook later
    return 0;
}

/* ---------------- WAKE ---------------- */

static wake_t detect_wake() {
    if (g_cfg.wiimote_enable && wiimote_present_stub())
        return WAKE_WIIMOTE;

    if (g_cfg.gamepad_enable)
        return WAKE_GAMEPAD;

    return WAKE_OTHER;
}

/* ---------------- BOOT ---------------- */

static const char *select_target(wake_t w) {
    if (w == WAKE_WIIMOTE && strlen(g_cfg.boot_wiimote))
        return g_cfg.boot_wiimote;

    if (w == WAKE_GAMEPAD && strlen(g_cfg.boot_gamepad))
        return g_cfg.boot_gamepad;

    if (strlen(g_cfg.boot_default))
        return g_cfg.boot_default;

    return g_cfg.fallback;
}

/* ---------------- MAIN ---------------- */

int plugin_main() {
    load_config();

    log_line("abw: start\n");

    wake_t w = detect_wake();

    if (safe_pressed_stub() && g_cfg.safe_mode) {
        log_line("abw: SAFE MODE\n");
        return 0;
    }

    OSSleepTicks(OSSecondsToTicks(g_cfg.timeout));

    const char *target = select_target(w);

    char out[300];
    snprintf(out, sizeof(out), "abw: boot -> %s\n", target);
    log_line(out);

    // Chainload comes later (Aroma API)

    return 0;
}
