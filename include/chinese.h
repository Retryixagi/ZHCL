#ifndef CHINESE_STABLE_H
#define CHINESE_STABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <locale.h>
#include <string>
#include <vector>
#include <string_view>
#include <cstring>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
static inline void 設定主控台UTF8(void)
{
  SetConsoleOutputCP(65001);
  SetConsoleCP(65001);
  _setmode(_fileno(stdout), _O_U8TEXT);
  _setmode(_fileno(stdin), _O_U8TEXT);
}
#else
static inline void 設定主控台UTF8(void) { (void)0; }
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
#ifdef _WIN32
  // 設定主控台為 UTF-8 以支援中文顯示
  system("chcp 65001 > nul");
#endif
}

// ---------------------------------------------
// 2) 輸出
// ---------------------------------------------
#define 輸出字串(s)    \
  do                   \
  {                    \
    printf("%s", (s)); \
  } while (0)
#define 輸出格式 printf

// ---------------------------------------------
// 3) 輸入
// ---------------------------------------------
static inline int 輸入整數(void)
{
  int v = 0;
#ifdef _MSC_VER
  if (scanf_s("%d", &v) != 1)
    return 0;
#else
  if (scanf("%d", &v) != 1)
    return 0;
#endif
  return v;
}

static inline double 輸入小數(void)
{
  double v = 0.0;
#ifdef _MSC_VER
  if (scanf_s("%lf", &v) != 1)
    return 0.0;
#else
  if (scanf("%lf", &v) != 1)
    return 0.0;
#endif
  return v;
}

// ---------------------------------------------
// 4) 隨機數
// ---------------------------------------------
#define 用時間當種子() srand((unsigned)time(NULL))
#define 當前秒() ((long)time(NULL))
#define 隨機數() (rand())

// ---------------------------------------------
// 5) 數學常數
// ---------------------------------------------
#ifndef 圓周率
#define 圓周率 3.14159265358979323846
#endif

// ---------------------------------------------
// 6) 泛型輸出 (C11)
// ---------------------------------------------
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define 輸出(x) _Generic((x),           \
    int: printf("%d\n", (x)),           \
    long: printf("%ld\n", (x)),         \
    long long: printf("%lld\n", (x)),   \
    unsigned: printf("%u\n", (x)),      \
    float: printf("%f\n", (double)(x)), \
    double: printf("%f\n", (x)),        \
    const char *: printf("%s", (x)),    \
    char *: printf("%s", (x)),          \
    default: printf("[不支援的型別]\n"))
#endif

// ---------------------------------------------
// 7) 中文關鍵字 (可選)
// ---------------------------------------------
#ifdef CHINESE_KEYWORDS
#define 整數 int
#define 小數 float
#define 雙精度小數 double
#define 布林 int
#define 真 1
#define 假 0
#define 當 while
#define 若 if
#define 否則 else
#define 開始 {
#define 結束 }
#define 返回 return
#endif

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

#ifdef CHINESE_KEYWORDS
#define 整數 int
#define 小數 float
#define 雙精度小數 double
#define 布林 int
#define 真 1
#define 假 0
#define 當 while
#define 若 if
#define 否則 else
#define 開始 {
#define 結束 }
#define 返回 return
#endif

#endif // CHINESE_STABLE_H
