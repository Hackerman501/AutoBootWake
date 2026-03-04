#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <coreinit/filesystem.h>
#include <coreinit/debug.h>
#include <coreinit/time.h>





/* ---------------- CONFIG ---------------- */

#define AUTOBOOT_PATH "/wiiu/environments/aroma/autoboot.cfg"
#define LOG_PATH      "/wiiu/abw.log"
#define SAFE_FILE     "/wiiu/SAFE"

#define TIMEOUT_MS 10000



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


/* ---------------- HELPERS ---------------- */

static int timeout_expired(uint64_t start)
{
    uint64_t now = OSGetTime();
    uint64_t diff = OSTicksToMilliseconds(now - start);
    return diff > TIMEOUT_MS;
}



static uint32_t get_power_cause(void)
{
    /* Not available in wut -> assume standby wake */
    return 1;
}



/* ---------------- ENTRY ---------------- */

int _start(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    uint64_t start_time = OSGetTime();
    char buf[256];
    char current[64] = {0};

    const char *target = "wiiu_menu\n";

    /* SAFE FILE */
    if (file_exists(SAFE_FILE)) {
        log_to_file("[autoboot] SAFE FILE\n");
        return 0;
    }

   

    uint32_t cause = get_power_cause();

    snprintf(buf, sizeof(buf), "[autoboot] cause=0x%08X\n", cause);
    log_to_file(buf);

    if (timeout_expired(start_time))
        return 0;

    /* SAFE POWER BUTTON */
    if (cause & 0x01) {
        log_to_file("[autoboot] SAFE POWER BUTTON\n");
        return 0;
    }

    /* Standby only */
    if (cause == 0) {
        log_to_file("[autoboot] NOT A WAKE EVENT\n");
        return 0;
    }

    /* WiiMote only */
    if (cause & 0x04) {
        log_to_file("[autoboot] WIIMOTE WAKE\n");
        target = "vwii\n";
    }

    if (timeout_expired(start_time))
        return 0;

    int r = read_file(AUTOBOOT_PATH, current, sizeof(current));

    if (r > 0 && strcmp(current, target) == 0) {
        log_to_file("[autoboot] ALREADY SET\n");
        return 0;
    }

    snprintf(buf, sizeof(buf), "[autoboot] WRITE %s", target);
    log_to_file(buf);

    write_file(AUTOBOOT_PATH, target);

    return 0;
}
