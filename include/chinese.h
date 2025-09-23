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
extern "C"
{
#endif

  // ---------------------------------------------
  // 1) 本地化 / 初始化
  // ---------------------------------------------
  static inline void 初始化中文環境(void)
  {
    // 先嘗試 UTF-8，失敗再回退到 Windows 舊名稱
    if (!setlocale(LC_ALL, "zh_TW.UTF-8"))
    {
      setlocale(LC_ALL, "Chinese_Traditional");
    }
  }

#ifdef _WIN32
#ifdef CHINESE_ENABLE_WINCP
// 抑制 MSVC C5105 警告（Windows SDK 巨集展開問題）
#pragma warning(push)
#pragma warning(disable : 5105)
#define WIN32_LEAN_AND_MEAN // 減少不必要的Windows標頭
#define NOMINMAX            // 避免min/max巨集衝突
#include <windows.h>
#pragma warning(pop)
  static inline void 設定主控台UTF8(void)
  {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
  }
#endif
#endif

// ---------------------------------------------
// 2) 常數（數學）
// ---------------------------------------------
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

#define 圓周率 M_PI
#define 自然常數 M_E

// ---------------------------------------------
// 3) 數學別名（直接映射到 <math.h>）
// ---------------------------------------------
#define 次冪 pow
#define 平方根 sqrt
#define 立方根 cbrt
#define 正弦 sin
#define 餘弦 cos
#define 正切 tan
#define 反正弦 asin
#define 反餘弦 acos
#define 反正切 atan
#define 雙變量反正切 atan2
#define 絕對值 fabs
#define 向下取整 floor
#define 向上取整 ceil
#define 四捨五入 round

  // ---------------------------------------------
  // 4) 隨機/時間 工具
  // ---------------------------------------------
  static inline void 設隨機種子(unsigned seed) { srand(seed); }
  static inline void 用時間當種子(void) { srand((unsigned)time(NULL)); }
  static inline int 隨機數(void) { return rand(); }
  static inline long 當前秒(void) { return (long)time(NULL); }

  // ---------------------------------------------
  // 5) 輸出（含格式化與 _Generic 多型）
  // ---------------------------------------------
  static inline void 輸出字串(const char *s) { printf("%s\n", s); }
  static inline void 印出(const char *s) { printf("%s", s); }

  static inline void 輸出格式(const char *fmt, ...)
  {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }

  static inline void 輸出整數(long long v) { printf("%lld\n", v); }
  static inline void 輸出無號(unsigned long long v) { printf("%llu\n", v); }
  static inline void 輸出小數(double v) { printf("%.15g\n", v); }
  static inline void 輸出布林(bool b) { printf("%s\n", b ? "真" : "假"); }

// C11 _Generic：為常見型別自動選擇輸出函式
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || \
    (defined(_MSC_VER) && _MSC_VER >= 1928) // VS 2019 16.8+ 大致可用
#define 輸出(x) _Generic((x), \
    char *: 輸出字串,         \
    const char *: 輸出字串,   \
    bool: 輸出布林,           \
    float: 輸出小數,          \
    double: 輸出小數,         \
    long double: 輸出小數,    \
    signed char: 輸出整數,    \
    short: 輸出整數,          \
    int: 輸出整數,            \
    long: 輸出整數,           \
    long long: 輸出整數,      \
    unsigned char: 輸出無號,  \
    unsigned short: 輸出無號, \
    unsigned int: 輸出無號,   \
    unsigned long: 輸出無號,  \
    unsigned long long: 輸出無號)(x)
#else
// 無 _Generic 時的後備：簡單的輸出宏
#define 輸出(x) 輸出小數(x) // 預設使用輸出小數，因為它可以處理多種類型
#endif

  // ---------------------------------------------
  // 6) 輸入
  // ---------------------------------------------
  static inline int 輸入整數(void)
  {
    int v;
    scanf("%d", &v);
    return v;
  }
  static inline double 輸入小數(void)
  {
    double v;
    if (scanf("%lf", &v) != 1)
    {
      // 如果讀取失敗，返回 0
      return 0.0;
    }
    return v;
  }
#define 輸入整數到(p) scanf("%d", (p))
#define 輸入小數到(p) scanf("%lf", (p))

// ---------------------------------------------
// 7)（可選）中文關鍵字映射
//     使用方法：在 *包含* 此檔案前定義 CHINESE_KEYWORDS
//     例如：cl /DCHINESE_KEYWORDS 或 gcc -DCHINESE_KEYWORDS
// ---------------------------------------------
#ifdef CHINESE_KEYWORDS
  // 型別與常量
#define 整數 int
#define 小數 float
#define 雙精度小數 double
#define 布林 bool
#define 字串 char *
#define 真 true
#define 假 false

  // 關鍵字/結構
#define 如果 if
#define 否則 else
#define 當 while
#define 對於           for
#define 做 do
#define 返回 return
#define 開始 {
#define 結束 }

  // 常見運算子別名（選用）
#define 加 +
#define 減 -
#define 乘 *
#define 除 /
#define 等於 ==
#define 不等於 !=
#define 大於 >
#define 小於 <
#define 大於等於 >=
#define 小於等於 <=
#endif // CHINESE_KEYWORDS

// ---------------------------------------------
// C++ 擴展功能（與 C 代碼兼容）
// ---------------------------------------------
#ifdef __cplusplus

#include <string>
#include <vector>
#include <string_view>
#include <cstring>
#include <cstdint>

  // Strip UTF-8 BOM from string
  void zh_strip_utf8_bom(char *s, size_t len);

  // Normalize newlines
  void zh_normalize_newlines(std::string &s);

  // NFKC normalization
  void zh_nfkc(std::string &s);

  // Simplify Chinese text
  void zh_simplify(std::string &s);

  // AUTO-GENERATED. Do not edit manually.
  // Generated from: all_symbols.csv
  // Total mappings: 26

  enum ZhKeywordKind : uint8_t
  {
    ZHK_WORD = 0,  // Regular keywords (if, for, int, etc.)
    ZHK_OP = 1,    // Operators (=, ==, etc.)
    ZHK_PUNCT = 2, // Punctuation-like ({, }, ", etc.)
    ZHK_LIT = 3    // Literals (numbers, strings, etc.)
  };

  struct ZhKeyword
  {
    std::string_view key;
    std::string_view map_to;
    ZhKeywordKind kind;
    float score;
    std::string_view tags;
  };

  static constexpr ZhKeyword ZH_KEYWORDS[] = {
      {"\\u7121\\u56de\\u50b3", "void", static_cast<ZhKeywordKind>(0), 1.0f, "type,builtin"},
      {"\\u4e3b\\u51fd\\u6578", "main", static_cast<ZhKeywordKind>(0), 1.0f, "type,builtin"},
      {"\\u986f\\u793a", "printf", static_cast<ZhKeywordKind>(0), 1.0f, "io"},
      {"\\u8f38\\u51fa", "printf", static_cast<ZhKeywordKind>(0), 1.0f, "io"},
      {"\\u5982\\u679c", "if", static_cast<ZhKeywordKind>(0), 1.0f, "control"},
      {"\\u5426\\u5247", "else", static_cast<ZhKeywordKind>(0), 1.0f, "control"},
      {"\\u4e0d\\u7136", "else", static_cast<ZhKeywordKind>(0), 1.0f, "control"},
      {"\\u91cd\\u8907", "for", static_cast<ZhKeywordKind>(0), 1.0f, "control"},
      {"\\u8ff4\\u5708", "for", static_cast<ZhKeywordKind>(0), 1.0f, "control"},
      {"\\u56de\\u50b3", "return", static_cast<ZhKeywordKind>(0), 1.0f, "control"},
      {"\\u6574\\u6578", "int", static_cast<ZhKeywordKind>(0), 1.0f, "type,builtin"},
      {"\\u8b8a\\u6578", "=", static_cast<ZhKeywordKind>(0), 0.9f, "assignment"},
      {"\\u8a2d\\u5b9a", "=", static_cast<ZhKeywordKind>(0), 0.9f, "assignment"},
      {"\\u90a3\\u9ebc", "{", static_cast<ZhKeywordKind>(2), 0.7f, "structure,danger"},
      {"\\u958b\\u59cb", "{", static_cast<ZhKeywordKind>(2), 0.7f, "structure,danger"},
      {"\\u7d50\\u675f", "}", static_cast<ZhKeywordKind>(2), 0.7f, "structure,danger"},
      {"\\u5b57\\u7b26\\u4e32", "char*", static_cast<ZhKeywordKind>(0), 1.0f, "type"},
      {"\\u5b57\\u7b26", "char", static_cast<ZhKeywordKind>(0), 1.0f, "type,builtin"},
      {"\\u9577\\u6574\\u6578", "long", static_cast<ZhKeywordKind>(0), 1.0f, "type,builtin"},
      {"\\u6d6e\\u9ede\\u6578", "float", static_cast<ZhKeywordKind>(0), 1.0f, "type,builtin"},
      {"\\u96d9\\u7cbe\\u5ea6\\u6d6e\\u9ede\\u6578", "double", static_cast<ZhKeywordKind>(0), 1.0f, "type,builtin"},
      {"\\u5e38\\u91cf", "const", static_cast<ZhKeywordKind>(0), 1.0f, "modifier"},
      {"\\u975c\\u614b", "static", static_cast<ZhKeywordKind>(0), 1.0f, "modifier"},
      {"\\u5916\\u90e8", "extern", static_cast<ZhKeywordKind>(0), 1.0f, "modifier"},
      {"\\u7a7a\\u9593", "void", static_cast<ZhKeywordKind>(0), 1.0f, "type,builtin"},
      {"\\u7d50\\u69cb", "struct", static_cast<ZhKeywordKind>(0), 1.0f, "type"},
  };

  static constexpr size_t ZH_KEYWORDS_COUNT = sizeof(ZH_KEYWORDS) / sizeof(ZH_KEYWORDS[0]);

#endif // __cplusplus

#ifdef __cplusplus
} // extern "C"
#endif
