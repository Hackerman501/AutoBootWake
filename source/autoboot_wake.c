#include <coreinit/filesystem.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/launch.h>
#include <sysapp/launch.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vpad/input.h>
#include <padscore/wpad.h>
#include <nn/cmpt.h>

#define CFG_PATH "/vol/storage_sdcard/wiiu/abw.cfg"
#define LOG_PATH "/vol/storage_sdcard/wiiu/abw.log"

typedef enum {
    TARGET_WIIU_MENU = 0,
    TARGET_VWII,
    TARGET_STAY
} target_t;

typedef enum {
    CONTROLLER_UNKNOWN = 0,
    CONTROLLER_GAMEPAD,
    CONTROLLER_WIIMOTE,
    CONTROLLER_PRO_CONTROLLER
} controller_type_t;

typedef struct {
    int timeout;
    int log;
    int safe_mode;
    int wiimote_enable;
    int gamepad_enable;
    char boot_default[64];
    char boot_gamepad[64];
    char boot_wiimote[64];
    char fallback[64];
} config_t;

static config_t g_cfg;
static int g_fs_initialized = 0;
static FSClient g_fs_client;
static FSCmdBlock g_fs_block;

static void init_fs() {
    if (g_fs_initialized) return;
    
    FSInit();
    FSInitCmdBlock(&g_fs_block);
    FSAddClient(&g_fs_client, FS_ERROR_FLAG_NONE);
    g_fs_initialized = 1;
}

static void log_line(const char *msg) {
    if (!g_cfg.log) return;

    init_fs();

    FSFileHandle fh;
    if (FSOpenFile(&g_fs_client, &g_fs_block, LOG_PATH, "a", &fh, -1) == FS_STATUS_OK) {
        FSWriteFile(&g_fs_client, &g_fs_block, (uint8_t*)msg, 1, strlen(msg), fh, 0, -1);
        FSCloseFile(&g_fs_client, &g_fs_block, fh, -1);
    }
}

static void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    size_t len = strlen(src);
    if (len >= dest_size) len = dest_size - 1;
    memcpy(dest, src, len);
    dest[len] = '\0';
}

static void cfg_defaults() {
    memset(&g_cfg, 0, sizeof(g_cfg));

    g_cfg.timeout = 5;
    g_cfg.log = 1;
    g_cfg.safe_mode = 1;
    g_cfg.wiimote_enable = 1;
    g_cfg.gamepad_enable = 1;

    safe_strcpy(g_cfg.boot_default, "wiiu_menu", sizeof(g_cfg.boot_default));
    safe_strcpy(g_cfg.boot_gamepad, "wiiu_menu", sizeof(g_cfg.boot_gamepad));
    safe_strcpy(g_cfg.boot_wiimote, "vwii", sizeof(g_cfg.boot_wiimote));
    safe_strcpy(g_cfg.fallback, "wiiu_menu", sizeof(g_cfg.fallback));
}

static void parse_line(const char *l) {
    if (l[0] == '#' || strlen(l) < 3) return;

    char key[64] = {0};
    char value[256] = {0};

    const char *eq = strchr(l, '=');
    if (!eq) return;

    size_t key_len = eq - l;
    if (key_len >= sizeof(key)) return;

    strncpy(key, l, key_len);
    strncpy(value, eq + 1, sizeof(value) - 1);

    char *end = value + strlen(value) - 1;
    while (end > value && (*end == '\r' || *end == '\n')) *end-- = '\0';

    if (strcmp(key, "timeout") == 0) g_cfg.timeout = atoi(value);
    else if (strcmp(key, "log") == 0) g_cfg.log = atoi(value);
    else if (strcmp(key, "safe_mode") == 0) g_cfg.safe_mode = atoi(value);
    else if (strcmp(key, "wiimote_enable") == 0) g_cfg.wiimote_enable = atoi(value);
    else if (strcmp(key, "gamepad_enable") == 0) g_cfg.gamepad_enable = atoi(value);
    else if (strcmp(key, "boot_default") == 0) safe_strcpy(g_cfg.boot_default, value, sizeof(g_cfg.boot_default));
    else if (strcmp(key, "boot_gamepad") == 0) safe_strcpy(g_cfg.boot_gamepad, value, sizeof(g_cfg.boot_gamepad));
    else if (strcmp(key, "boot_wiimote") == 0) safe_strcpy(g_cfg.boot_wiimote, value, sizeof(g_cfg.boot_wiimote));
    else if (strcmp(key, "fallback") == 0) safe_strcpy(g_cfg.fallback, value, sizeof(g_cfg.fallback));
}

static void load_config() {
    cfg_defaults();
    init_fs();

    FSFileHandle fh;
    if (FSOpenFile(&g_fs_client, &g_fs_block, CFG_PATH, "r", &fh, -1) != FS_STATUS_OK) {
        log_line("abw: no cfg, using defaults\n");
        return;
    }

    char buf[512];
    int bytes_read;
    char remaining[256] = {0};

    while ((bytes_read = FSReadFile(&g_fs_client, &g_fs_block, (uint8_t*)buf, 1, sizeof(buf) - 1, fh, 0, -1)) > 0) {
        buf[bytes_read] = '\0';

        if (remaining[0] != '\0') {
            strncat(remaining, buf, sizeof(remaining) - strlen(remaining) - 1);
            parse_line(remaining);
            remaining[0] = '\0';
        }

        char *line_start = buf;
        char *line_end;

        while ((line_end = strchr(line_start, '\n')) != NULL) {
            *line_end = '\0';
            parse_line(line_start);
            line_start = line_end + 1;
        }

        if (line_start < buf + bytes_read) {
            strncpy(remaining, line_start, sizeof(remaining) - 1);
        }

        if (bytes_read < sizeof(buf) - 1) break;
    }

    if (remaining[0] != '\0') {
        parse_line(remaining);
    }

    FSCloseFile(&g_fs_client, &g_fs_block, fh, -1);
}

static target_t parse_target(const char *t) {
    if (strcmp(t, "vwii") == 0) return TARGET_VWII;
    if (strcmp(t, "stay") == 0) return TARGET_STAY;
    return TARGET_WIIU_MENU;
}

static const char *target_name(target_t t) {
    if (t == TARGET_VWII) return "vwii";
    if (t == TARGET_STAY) return "stay";
    return "wiiu_menu";
}

static controller_type_t detect_controller() {
    VPADStatus vpad_status;
    int vpad_error;
    
    VPADInit();
    VPADRead(0, &vpad_status, 1, &vpad_error);
    
    if (vpad_error == 0 && (vpad_status.hold & VPAD_BUTTON_HOME) == 0) {
        log_line("abw: GamePad detected\n");
        VPADShutdown();
        return CONTROLLER_GAMEPAD;
    }
    VPADShutdown();
    
    WPADInit();
    
    for (int chan = 0; chan < 4; chan++) {
        WPADExtensionType ext_type;
        int result = WPADProbe(chan, &ext_type);
        
        if (result == WPAD_ERROR_NONE && ext_type != WPAD_EXT_CORE) {
            log_line("abw: WiiMote detected\n");
            WPADShutdown();
            return CONTROLLER_WIIMOTE;
        }
    }
    
    WPADShutdown();
    
    log_line("abw: no controller detected\n");
    return CONTROLLER_UNKNOWN;
}

static target_t determine_target(controller_type_t ctrl) {
    if (ctrl == CONTROLLER_WIIMOTE) {
        if (g_cfg.wiimote_enable) {
            log_line("abw: using wiimote target\n");
            return parse_target(g_cfg.boot_wiimote);
        }
    }
    else if (ctrl == CONTROLLER_GAMEPAD) {
        if (g_cfg.gamepad_enable) {
            log_line("abw: using gamepad target\n");
            return parse_target(g_cfg.boot_gamepad);
        }
    }
    
    log_line("abw: using default target\n");
    return parse_target(g_cfg.boot_default);
}

static void boot_to_target(target_t tgt) {
    char out[128];
    
    switch (tgt) {
        case TARGET_VWII:
            log_line("abw: booting to vWii\n");
            snprintf(out, sizeof(out), "abw: CMPT launch vWii\n");
            log_line(out);
            CMPTLaunchTitle(NULL, 0, 0x10004000ULL);
            break;
            
        case TARGET_WIIU_MENU:
            log_line("abw: booting to Wii U Menu\n");
            snprintf(out, sizeof(out), "abw: SYS launch menu 0x10040000\n");
            log_line(out);
            SYSLaunchTitle(0x10040000ULL);
            break;
            
        case TARGET_STAY:
        default:
            log_line("abw: staying in current title\n");
            break;
    }
}

int plugin_main() {
    load_config();

    log_line("abw: start\n");

    controller_type_t ctrl = detect_controller();
    target_t tgt = determine_target(ctrl);

    char out[128];
    snprintf(out, sizeof(out), "abw: waiting %d seconds\n", g_cfg.timeout);
    log_line(out);

    OSSleepTicks(OSSecondsToTicks(g_cfg.timeout));

    snprintf(out, sizeof(out), "abw: selected target = %s\n", target_name(tgt));
    log_line(out);

    boot_to_target(tgt);

    return 0;
}
