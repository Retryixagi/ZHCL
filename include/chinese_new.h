#pragma once
// chinese.h — Chinese to C bridge layer (enhanced version)
// - Math aliases, formatted output, random/time utilities
// - Optional Chinese keywords macros (enable with CHINESE_KEYWORDS)
// - Optional Windows console UTF-8 (enable with CHINESE_ENABLE_WINCP)

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------
// 1) Localization / Initialization
// ---------------------------------------------
static void initialize_chinese_environment(void) {
#ifdef _WIN32
    // Set console to UTF-8 on Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // Set locale for proper number formatting
    setlocale(LC_ALL, "");
}

#ifdef __cplusplus
}
#endif