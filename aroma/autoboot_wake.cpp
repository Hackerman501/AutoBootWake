// AutoBootWake - Controller-based Boot Selection for Aroma

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <coreinit/debug.h>
#include <coreinit/filesystem.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/title.h>
#include <sysapp/launch.h>
#include <nn/cmpt.h>
#include <vpad/input.h>
#include <padscore/wpad.h>
#include <padscore/kpad.h>

#define CFG_PATH "/vol/storage_sdcard/wiiu/abw.cfg"
#define LOG_PATH "/vol/storage_sdcard/wiiu/abw.log"

#define DEBUG(fmt, ...) OSReport("[AutoBootWake] " fmt, ##__VA_ARGS__)

typedef enum {
    TARGET_WIIU_MENU = 0,
    TARGET_VWII,
    TARGET_STAY
} target_t;

typedef enum {
    CONTROLLER_UNKNOWN = 0,
    CONTROLLER_GAMEPAD,
    CONTROLLER_WIIMOTE
} controller_type_t;

typedef struct {
    int timeout;
    int log;
    int wiimote_enable;
    int gamepad_enable;
    char boot_default[64];
    char boot_gamepad[64];
    char boot_wiimote[64];
} config_t;

static config_t g_cfg;
static FSClient g_fs_client;
static FSCmdBlock g_fs_block;
static int g_fs_initialized = 0;

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
    if (FSOpenFile(&g_fs_client, &g_fs_block, LOG_PATH, "a", &fh, FS_ERROR_FLAG_NONE) == FS_STATUS_OK) {
        FSWriteFile(&g_fs_client, &g_fs_block, (uint8_t*)msg, 1, strlen(msg), fh, 0, FS_ERROR_FLAG_NONE);
        FSCloseFile(&g_fs_client, &g_fs_block, fh, FS_ERROR_FLAG_NONE);
    }
}

static void cfg_defaults() {
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.timeout = 5;
    g_cfg.log = 1;
    g_cfg.wiimote_enable = 1;
    g_cfg.gamepad_enable = 1;
    strcpy(g_cfg.boot_default, "wiiu_menu");
    strcpy(g_cfg.boot_gamepad, "wiiu_menu");
    strcpy(g_cfg.boot_wiimote, "vwii");
}

static void load_config() {
    cfg_defaults();
    init_fs();

    FSFileHandle fh;
    if (FSOpenFile(&g_fs_client, &g_fs_block, CFG_PATH, "r", &fh, FS_ERROR_FLAG_NONE) != FS_STATUS_OK) {
        log_line("AutoBootWake: no cfg, using defaults\n");
        return;
    }

    char buf[512];
    int bytes_read;
    while ((bytes_read = FSReadFile(&g_fs_client, &g_fs_block, (uint8_t*)buf, 1, sizeof(buf)-1, fh, 0, FS_ERROR_FLAG_NONE)) > 0) {
        buf[bytes_read] = '\0';
        char *line = strtok(buf, "\n");
        while (line) {
            if (line[0] != '#' && strlen(line) >= 3) {
                char key[64] = {0}, value[256] = {0};
                char *eq = strchr(line, '=');
                if (eq) {
                    size_t key_len = eq - line;
                    if (key_len < sizeof(key)) {
                        strncpy(key, line, key_len);
                        strncpy(value, eq + 1, sizeof(value) - 1);
                        if (strcmp(key, "timeout") == 0) g_cfg.timeout = atoi(value);
                        else if (strcmp(key, "log") == 0) g_cfg.log = atoi(value);
                        else if (strcmp(key, "wiimote_enable") == 0) g_cfg.wiimote_enable = atoi(value);
                        else if (strcmp(key, "gamepad_enable") == 0) g_cfg.gamepad_enable = atoi(value);
                        else if (strcmp(key, "boot_default") == 0) strncpy(g_cfg.boot_default, value, 63);
                        else if (strcmp(key, "boot_gamepad") == 0) strncpy(g_cfg.boot_gamepad, value, 63);
                        else if (strcmp(key, "boot_wiimote") == 0) strncpy(g_cfg.boot_wiimote, value, 63);
                    }
                }
            }
            line = strtok(NULL, "\n");
        }
    }
    FSCloseFile(&g_fs_client, &g_fs_block, fh, FS_ERROR_FLAG_NONE);
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
    VPADReadError vpad_error;
    
    VPADInit();
    VPADRead(VPAD_CHAN_0, &vpad_status, 1, &vpad_error);
    
    if (vpad_error == 0 && vpad_status.hold != 0) {
        DEBUG("GamePad detected, hold=0x%08X\n", vpad_status.hold);
        VPADShutdown();
        return CONTROLLER_GAMEPAD;
    }
    VPADShutdown();
    
    WPADInit();
    for (int chan = 0; chan < 4; chan++) {
        WPADExtensionType ext_type;
        WPADError result = WPADProbe((WPADChan)chan, &ext_type);
        if (result == WPAD_ERROR_NONE && ext_type != WPAD_EXT_CORE && ext_type != 0) {
            DEBUG("WiiMote detected on channel %d, ext_type=%d\n", chan, ext_type);
            WPADShutdown();
            return CONTROLLER_WIIMOTE;
        }
    }
    WPADShutdown();
    
    return CONTROLLER_UNKNOWN;
}

static target_t determine_target(controller_type_t ctrl) {
    if (ctrl == CONTROLLER_WIIMOTE && g_cfg.wiimote_enable) {
        log_line("AutoBootWake: using wiimote target\n");
        return parse_target(g_cfg.boot_wiimote);
    }
    if (ctrl == CONTROLLER_GAMEPAD && g_cfg.gamepad_enable) {
        log_line("AutoBootWake: using gamepad target\n");
        return parse_target(g_cfg.boot_gamepad);
    }
    log_line("AutoBootWake: using default target\n");
    return parse_target(g_cfg.boot_default);
}

static void boot_to_target(target_t tgt) {
    switch (tgt) {
        case TARGET_VWII:
            DEBUG("AutoBootWake: Booting to vWii...\n");
            log_line("AutoBootWake: booting to vWii\n");
            KPADInit();
            CMPTAcctSetScreenType(CMPT_SCREEN_TYPE_BOTH);
            CMPTLaunchTitle(NULL, 0, 0x10004000ULL);
            break;
            
        case TARGET_WIIU_MENU:
            DEBUG("AutoBootWake: Booting to Wii U Menu...\n");
            log_line("AutoBootWake: booting to Wii U Menu\n");
            SYSLaunchMenu();
            break;
            
        case TARGET_STAY:
        default:
            DEBUG("AutoBootWake: Staying in current title\n");
            log_line("AutoBootWake: staying\n");
            break;
    }
}

int main(int argc, char **argv) {
    DEBUG("AutoBootWake v1.0 - Starting...\n");
    
    load_config();
    log_line("AutoBootWake: start\n");
    
    DEBUG("Detecting controller...\n");
    controller_type_t ctrl = detect_controller();
    
    target_t tgt = determine_target(ctrl);
    
    DEBUG("Selected target: %s\n", target_name(tgt));
    
    if (g_cfg.timeout > 0) {
        DEBUG("Waiting %d seconds...\n", g_cfg.timeout);
        OSSleepTicks(OSSecondsToTicks(g_cfg.timeout));
    }
    
    boot_to_target(tgt);
    
    DEBUG("AutoBootWake: Done\n");
    return 0;
}
