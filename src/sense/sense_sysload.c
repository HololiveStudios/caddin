/* sense_sysload.c — CPU load, temperatura e pressão de memória */
#define _POSIX_C_SOURCE 200809L
#include "sense_sysload.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define UPDATE_INTERVAL 1.0  /* segundos entre leituras */

void sense_sysload_init(SenseSysload *s) {
    memset(s, 0, sizeof(*s));
    s->last_update = -9999.0;
    s->prev_idle   = 0;
    s->prev_total  = 0;
}

#if defined(__linux__) && !defined(_WIN32)

/* Lê /proc/stat linha cpu e devolve (idle, total) */
static bool read_proc_stat(long long *idle_out, long long *total_out) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return false;
    char line[256];
    bool ok = false;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "cpu ", 4) == 0) {
            long long user, nice, sys, idle, iowait, irq, softirq, steal;
            if (sscanf(line + 4, "%lld %lld %lld %lld %lld %lld %lld %lld",
                       &user, &nice, &sys, &idle, &iowait, &irq, &softirq, &steal) == 8) {
                *idle_out  = idle + iowait;
                *total_out = user + nice + sys + idle + iowait + irq + softirq + steal;
                ok = true;
            }
            break;
        }
    }
    fclose(f);
    return ok;
}

/* Lê temperatura via hwmon (primeiro thermal_zone válido) */
static float read_cpu_temp(void) {
    /* Tenta /sys/class/thermal/thermal_zone0/temp */
    for (int i = 0; i < 4; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", i);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        int millic = 0;
        fscanf(f, "%d", &millic);
        fclose(f);
        if (millic > 0) return (float)millic / 1000.0f;  /* graus C */
    }
    return 50.0f;  /* padrão seguro */
}

/* Lê memória livre/total de /proc/meminfo */
static float read_mem_pressure(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0.5f;
    char line[128];
    long long total = 0, available = 0;
    while (fgets(line, sizeof(line), f)) {
        long long val;
        if (sscanf(line, "MemTotal: %lld", &val) == 1) total = val;
        if (sscanf(line, "MemAvailable: %lld", &val) == 1) available = val;
        if (total && available) break;
    }
    fclose(f);
    if (total <= 0) return 0.5f;
    float free_ratio = (float)available / (float)total;
    return 1.0f - free_ratio;  /* 1 = memória cheia (pressão alta) */
}

bool sense_sysload_update(SenseSysload *s, double now_sec) {
    if (now_sec - s->last_update < UPDATE_INTERVAL) return false;
    s->last_update = now_sec;

    /* CPU load */
    long long idle = 0, total = 0;
    if (read_proc_stat(&idle, &total) && s->prev_total > 0) {
        long long d_idle  = idle  - s->prev_idle;
        long long d_total = total - s->prev_total;
        if (d_total > 0) {
            float load = 1.0f - (float)d_idle / (float)d_total;
            if (load < 0.0f) load = 0.0f;
            if (load > 1.0f) load = 1.0f;
            s->cpu_load = fix_from_float(load);
        }
    }
    s->prev_idle  = idle;
    s->prev_total = total;

    /* Temperatura — normalizada: 20°C=0, 100°C=1 */
    float temp_c = read_cpu_temp();
    float tn = (temp_c - 20.0f) / 80.0f;
    if (tn < 0.0f) tn = 0.0f;
    if (tn > 1.0f) tn = 1.0f;
    s->cpu_temp = fix_from_float(tn);

    /* Pressão de memória */
    float mp = read_mem_pressure();
    if (mp < 0.0f) mp = 0.0f;
    if (mp > 1.0f) mp = 1.0f;
    s->mem_pressure = fix_from_float(mp);

    return true;
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static FILETIME g_prev_idle = {0}, g_prev_kernel = {0}, g_prev_user = {0};
static bool     g_first = true;

static ULONGLONG ft_to_ull(FILETIME ft) {
    return ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}

bool sense_sysload_update(SenseSysload *s, double now_sec) {
    if (now_sec - s->last_update < UPDATE_INTERVAL) return false;
    s->last_update = now_sec;

    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        if (!g_first) {
            ULONGLONG d_idle   = ft_to_ull(idle)   - ft_to_ull(g_prev_idle);
            ULONGLONG d_kernel = ft_to_ull(kernel) - ft_to_ull(g_prev_kernel);
            ULONGLONG d_user   = ft_to_ull(user)   - ft_to_ull(g_prev_user);
            ULONGLONG d_total  = d_kernel + d_user;
            if (d_total > 0) {
                float load = 1.0f - (float)d_idle / (float)d_total;
                if (load < 0.0f) load = 0.0f;
                if (load > 1.0f) load = 1.0f;
                s->cpu_load = fix_from_float(load);
            }
        }
        g_prev_idle   = idle;
        g_prev_kernel = kernel;
        g_prev_user   = user;
        g_first = false;
    }

    /* Memória */
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        float mp = (float)ms.dwMemoryLoad / 100.0f;
        s->mem_pressure = fix_from_float(mp);
    }

    s->cpu_temp = fix_from_float(0.4f);  /* Sem API padrao no Windows */
    return true;
}

#else

bool sense_sysload_update(SenseSysload *s, double now_sec) {
    (void)s; (void)now_sec; return false;
}

#endif /* platform */

fix16_t sense_sysload_get_cpu(const SenseSysload *s)  { return s->cpu_load; }
fix16_t sense_sysload_get_temp(const SenseSysload *s) { return s->cpu_temp; }
fix16_t sense_sysload_get_mem(const SenseSysload *s)  { return s->mem_pressure; }
