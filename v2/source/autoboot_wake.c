#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>


#include <coreinit/filesystem.h>
#include <coreinit/debug.h>
#include <coreinit/time.h>

/* ---------------- PATHS ---------------- */

#define AUTOBOOT_PATH "/wiiu/environments/aroma/autoboot.cfg"
#define LOG_PATH      "/wiiu/abw.log"
#define SAFE_FILE     "/wiiu/SAFE"
#define CFG_PATH      "/wiiu/abw.cfg"

/* ---------------- DEFAULTS ---------------- */

#define DEF_TIMEOUT 10000
#define DEF_DEFAULT "wiiu_menu\n"
#define DEF_WIIMOTE "vwii\n"

/* ---------------- CONFIG ---------------- */

typedef struct {
    int timeout;
    int safe;
    int log;
    char def_boot[32];
    char wiimote_boot[32];
} ABWConfig;


/* ---------------- FS HELPERS ---------------- */

static void log_to_file(const char *msg)
{
    FSClient client;
    FSCmdBlock block;
    FSFileHandle fd;

    FSInit();
    FSAddClient(&client, FS_ERROR_FLAG_ALL);
    FSInitCmdBlock(&block);

    if (FSOpenFile(&client, &block, LOG_PATH, "a", &fd, -1) < 0)
        return;

    FSWriteFile(&client, &block, (uint8_t *)msg, 1, strlen(msg), fd, 0, -1);
    FSCloseFile(&client, &block, fd, -1);
}

static int file_exists(const char *path)
{
    FSClient client;
    FSCmdBlock block;
    FSFileHandle fd;

    FSInit();
    FSAddClient(&client, FS_ERROR_FLAG_ALL);
    FSInitCmdBlock(&block);

    if (FSOpenFile(&client, &block, path, "r", &fd, -1) < 0)
        return 0;

    FSCloseFile(&client, &block, fd, -1);
    return 1;
}

static int read_file(const char *path, char *out, int max)
{
    FSClient client;
    FSCmdBlock block;
    FSFileHandle fd;

    FSInit();
    FSAddClient(&client, FS_ERROR_FLAG_ALL);
    FSInitCmdBlock(&block);

    if (FSOpenFile(&client, &block, path, "r", &fd, -1) < 0)
        return -1;

    int r = FSReadFile(&client, &block, (uint8_t *)out, 1, max - 1, fd, 0, -1);
    FSCloseFile(&client, &block, fd, -1);

    if (r > 0)
        out[r] = 0;

    return r;
}

static int write_file(const char *path, const char *data)
{
    FSClient client;
    FSCmdBlock block;
    FSFileHandle fd;

    FSInit();
    FSAddClient(&client, FS_ERROR_FLAG_ALL);
    FSInitCmdBlock(&block);

    if (FSOpenFile(&client, &block, path, "w", &fd, -1) < 0)
        return -1;

    FSWriteFile(&client, &block, (uint8_t *)data, 1, strlen(data), fd, 0, -1);
    FSCloseFile(&client, &block, fd, -1);

    return 0;
}


/* ---------------- CONFIG PARSER ---------------- */

static void cfg_defaults(ABWConfig *c)
{
    c->timeout = DEF_TIMEOUT;
    c->safe = 1;
    c->log = 1;
    strcpy(c->def_boot, DEF_DEFAULT);
    strcpy(c->wiimote_boot, DEF_WIIMOTE);
}

static void parse_line(ABWConfig *c, char *l)
{
    char *eq = strchr(l, '=');
    if (!eq) return;

    *eq = 0;
    char *k = l;
    char *v = eq + 1;

    if (!strcmp(k, "timeout")) c->timeout = atoi(v);
    else if (!strcmp(k, "safe")) c->safe = atoi(v);
    else if (!strcmp(k, "log")) c->log = atoi(v);
    else if (!strcmp(k, "default")) {
        snprintf(c->def_boot, sizeof(c->def_boot), "%s\n", v);
    }
    else if (!strcmp(k, "wiimote")) {
        snprintf(c->wiimote_boot, sizeof(c->wiimote_boot), "%s\n", v);
    }
}

static void load_config(ABWConfig *c)
{
    char buf[512];

    cfg_defaults(c);

    if (read_file(CFG_PATH, buf, sizeof(buf)) <= 0)
        return;

    char *l = strtok(buf, "\n\r");
    while (l) {
        parse_line(c, l);
        l = strtok(NULL, "\n\r");
    }
}


/* ---------------- UTILS ---------------- */

static int timeout_expired(uint64_t start, int limit)
{
    uint64_t now = OSGetTime();
    uint64_t diff = OSTicksToMilliseconds(now - start);
    return diff > (uint64_t)limit;
}


/* ---------------- ENTRY ---------------- */

int _start(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    ABWConfig cfg;
    load_config(&cfg);

    uint64_t start_time = OSGetTime();
    char buf[128];
    char current[64] = {0};

    if (cfg.safe && file_exists(SAFE_FILE)) {
        if (cfg.log) log_to_file("[abw] SAFE FILE\n");
        return 0;
    }

    if (timeout_expired(start_time, cfg.timeout))
        return 0;

    read_file(AUTOBOOT_PATH, current, sizeof(current));

    if (!strcmp(current, cfg.def_boot)) {
        if (cfg.log) log_to_file("[abw] ALREADY SET\n");
        return 0;
    }

    if (cfg.log) {
        snprintf(buf, sizeof(buf), "[abw] WRITE %s", cfg.def_boot);
        log_to_file(buf);
    }

    write_file(AUTOBOOT_PATH, cfg.def_boot);

    return 0;
}
