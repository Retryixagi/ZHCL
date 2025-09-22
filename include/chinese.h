#pragma once
// =============================================================
// chinese.h — 繁體中文 → C 的輕量橋接層（增強版）
// - 數學別名、格式化輸出、隨機/時間工具
// - 可選「中文關鍵字」巨集（以 CHINESE_KEYWORDS 啟用）
// - 可選 Windows 主控台 UTF‑8（以 CHINESE_ENABLE_WINCP 啟用）
// =============================================================

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

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------
// 1) 本地化 / 初始化
// ---------------------------------------------
static inline void 初始化中文環境(void) {
    // 先嘗試 UTF-8，失敗再回退到 Windows 舊名稱
    if (!setlocale(LC_ALL, "zh_TW.UTF-8")) {
        setlocale(LC_ALL, "Chinese_Traditional");
    }
}

#ifdef _WIN32
#  ifdef CHINESE_ENABLE_WINCP
// 抑制 MSVC C5105 警告（Windows SDK 巨集展開問題）
#    pragma warning(push)
#    pragma warning(disable:5105)
#    define WIN32_LEAN_AND_MEAN  // 減少不必要的Windows標頭
#    define NOMINMAX           // 避免min/max巨集衝突
#    include <windows.h>
#    pragma warning(pop)
static inline void 設定主控台UTF8(void) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
}
#  endif
#endif

// ---------------------------------------------
// 2) 常數（數學）
// ---------------------------------------------
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#  define M_E  2.71828182845904523536
#endif

#define 圓周率     M_PI
#define 自然常數   M_E

// ---------------------------------------------
// 3) 數學別名（直接映射到 <math.h>）
// ---------------------------------------------
#define 次冪        pow
#define 平方根      sqrt
#define 立方根      cbrt
#define 正弦        sin
#define 餘弦        cos
#define 正切        tan
#define 反正弦      asin
#define 反餘弦      acos
#define 反正切      atan
#define 雙變量反正切 atan2
#define 絕對值      fabs
#define 向下取整    floor
#define 向上取整    ceil
#define 四捨五入    round

// ---------------------------------------------
// 4) 隨機/時間 工具
// ---------------------------------------------
static inline void 設隨機種子(unsigned seed) { srand(seed); }
static inline void 用時間當種子(void) { srand((unsigned)time(NULL)); }
static inline int  隨機數(void) { return rand(); }
static inline long 當前秒(void) { return (long)time(NULL); }

// ---------------------------------------------
// 5) 輸出（含格式化與 _Generic 多型）
// ---------------------------------------------
static inline void 輸出字串(const char *s) { printf("%s\n", s); }
static inline void 印出(const char *s)     { printf("%s", s); }

static inline void 輸出格式(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
}

static inline void 輸出整數(long long v)   { printf("%lld\n", v); }
static inline void 輸出無號(unsigned long long v) { printf("%llu\n", v); }
static inline void 輸出小數(double v)      { printf("%.15g\n", v); }
static inline void 輸出布林(bool b)        { printf("%s\n", b ? "真" : "假"); }

// C11 _Generic：為常見型別自動選擇輸出函式
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || \
    (defined(_MSC_VER) && _MSC_VER >= 1928)  // VS 2019 16.8+ 大致可用
#  define 輸出(x) _Generic((x), \
        char*:               輸出字串, \
        const char*:         輸出字串, \
        bool:                輸出布林, \
        float:               輸出小數, \
        double:              輸出小數, \
        long double:         輸出小數, \
        signed char:         輸出整數, \
        short:               輸出整數, \
        int:                 輸出整數, \
        long:                輸出整數, \
        long long:           輸出整數, \
        unsigned char:       輸出無號, \
        unsigned short:      輸出無號, \
        unsigned int:        輸出無號, \
        unsigned long:       輸出無號, \
        unsigned long long:  輸出無號 \
    )(x)
#else
// 無 _Generic 時的後備：簡單的輸出宏
#  define 輸出(x) 輸出小數(x)  // 預設使用輸出小數，因為它可以處理多種類型
#endif

// ---------------------------------------------
// 6) 輸入
// ---------------------------------------------
static inline int    輸入整數(void) { int v;    scanf("%d",  &v); return v; }
static inline double 輸入小數(void) {
    double v;
    if (scanf("%lf", &v) != 1) {
        // 如果讀取失敗，返回 0
        return 0.0;
    }
    return v;
}
#define 輸入整數到(p)  scanf("%d",  (p))
#define 輸入小數到(p)  scanf("%lf", (p))

// ---------------------------------------------
// 7)（可選）中文關鍵字映射
//     使用方法：在 *包含* 此檔案前定義 CHINESE_KEYWORDS
//     例如：cl /DCHINESE_KEYWORDS 或 gcc -DCHINESE_KEYWORDS
// ---------------------------------------------
#ifdef CHINESE_KEYWORDS
  // 型別與常量
# define 整數           int
# define 小數           float
# define 雙精度小數     double
# define 布林           bool
# define 字串           char*
# define 真             true
# define 假             false

  // 關鍵字/結構
# define 如果           if
# define 否則           else
# define 當             while
# define 對於           for
# define 做             do
# define 返回           return
# define 開始           {
# define 結束           }

  // 常見運算子別名（選用）
# define 加 +
# define 減 -
# define 乘 *
# define 除 /
# define 等於 ==
# define 不等於 !=
# define 大於 >
# define 小於 <
# define 大於等於 >=
# define 小於等於 <=
#endif // CHINESE_KEYWORDS

#ifdef __cplusplus
} // extern "C"
#endif
