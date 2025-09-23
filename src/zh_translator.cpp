#include "../include/zh_matrix.inc"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <cctype>
#include <cstring>
#include <string_view>
#include <map>
#include <cstdint>

// Simplified ZhKeyword structure for standalone translator
enum ZhKeywordKind
{
  ZHK_WORD = 0,
  ZHK_OP = 1,
  ZHK_PUNCT = 2,
  ZHK_LIT = 3
};

struct ZhKeyword
{
  const char *key;
  const char *map_to;
  ZhKeywordKind kind;
  float score;
  const char *tags;
};

// Include the keyword definitions from chinese.h
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
    {"\\u5b9a\\u7fa9", "int", static_cast<ZhKeywordKind>(0), 0.9f, "type,declaration"},
    {"\\u8b8a\\u6578", "=", static_cast<ZhKeywordKind>(0), 0.9f, "assignment"},
    {"\\u8a2d\\u5b9a", "=", static_cast<ZhKeywordKind>(0), 0.9f, "assignment"},
    {"\\u8a2d\\u70ba", "=", static_cast<ZhKeywordKind>(0), 0.9f, "assignment"},
    {"\\u8a2d\\u70ba", "=", static_cast<ZhKeywordKind>(0), 0.9f, "assignment"},
    {"\\u90a3\\u9ebc", "{", static_cast<ZhKeywordKind>(2), 0.7f, "structure,danger"},
    {"\\u958b\\u59cb", "{", static_cast<ZhKeywordKind>(2), 0.7f, "structure,danger"},
    {"\\u7d50\\u675f", "}", static_cast<ZhKeywordKind>(2), 0.7f, "structure,danger"},
    {"\\u5b57\\u4e32", "char*", static_cast<ZhKeywordKind>(0), 1.0f, "type"},
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

// Forward declaration
std::string rewrite_with_matrix(const std::string &src);

// Helper functions
static inline void zh_strip_utf8_bom(std::string &s)
{
  if (s.size() >= 3 &&
      (unsigned char)s[0] == 0xEF &&
      (unsigned char)s[1] == 0xBB &&
      (unsigned char)s[2] == 0xBF)
  {
    s.erase(0, 3);
  }
}

static inline void zh_normalize_newlines(std::string &s)
{
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i)
  {
    char c = s[i];
    if (c == '\r')
    {
      if (i + 1 < s.size() && s[i + 1] == '\n')
        ++i;
      out.push_back('\n');
    }
    else
      out.push_back(c);
  }
  s.swap(out);
}

static inline void zh_nfkc(std::string &s)
{
  // Stub for Unicode NFKC normalization
}

static inline void zh_simplify(std::string &s)
{
  // Stub for traditional to simplified conversion
}

static inline bool is_ident_char(unsigned char c)
{
  return isalnum(c) || c == '_' || (c >= 128);
}

static inline void u8(std::vector<uint8_t> &bc, unsigned v) { bc.push_back((uint8_t)v); }
static inline void u64le(std::vector<uint8_t> &bc, uint64_t v)
{
  for (int i = 0; i < 8; i++)
    bc.push_back((uint8_t)((v >> (8 * i)) & 0xFF));
}

static inline void i64le(std::vector<uint8_t> &bc, int64_t v) { u64le(bc, (uint64_t)v); }

// Main rewriting function
std::string rewrite_with_matrix(const std::string &src)
{
  std::string s = src;

  // Preprocessing
  zh_strip_utf8_bom(s);
  zh_normalize_newlines(s);
  zh_nfkc(s);
  // zh_simplify(s); // Disabled: keep traditional Chinese as requested

  size_t n = s.size();
  std::vector<int> dp(n + 1, INT_MIN);
  std::vector<size_t> prev(n + 1, SIZE_MAX);
  std::vector<std::string> replacement(n + 1, "");
  std::vector<uint8_t> kind_history(n + 1, 0);

  dp[0] = 0;
  kind_history[0] = 0;

  // Initialize fallback
  for (size_t j = 1; j <= n; ++j)
  {
    if (dp[j - 1] > dp[j])
    {
      dp[j] = dp[j - 1];
      prev[j] = j - 1;
      replacement[j] = std::string(1, s[j - 1]);
    }
  }

  // Helper: decode Unicode escape sequences to UTF-8
  auto decode_unicode_escapes = [](const std::string_view &input) -> std::string
  {
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i)
    {
      if (i + 5 < input.size() && input[i] == '\\' && input[i + 1] == 'u')
      {
        char hex[5] = {input[i + 2], input[i + 3], input[i + 4], input[i + 5], '\0'};
        unsigned int codepoint = strtoul(hex, nullptr, 16);

        if (codepoint <= 0x7F)
        {
          result.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint <= 0x7FF)
        {
          result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
          result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint <= 0xFFFF)
        {
          result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
          result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else
        {
          result.push_back('?');
        }
        i += 5;
      }
      else
      {
        result.push_back(input[i]);
      }
    }
    return result;
  };

  // Build DP table
  for (size_t i = 0; i < n; ++i)
  {
    if (dp[i] == INT_MIN)
      continue;

    // Check if inside string
    bool in_string = false;
    char string_quote = 0;
    bool escaped = false;

    for (size_t j = 0; j < i; ++j)
    {
      char c = s[j];
      if (escaped)
      {
        escaped = false;
        continue;
      }
      if (c == '\\')
      {
        escaped = true;
        continue;
      }
      if (!in_string && (c == '"' || c == '\''))
      {
        in_string = true;
        string_quote = c;
      }
      else if (in_string && c == string_quote)
      {
        in_string = false;
        string_quote = 0;
      }
    }

    if (in_string)
      continue;

    // Try all keywords
    for (size_t ki = 0; ki < ZH_KEYWORDS_COUNT; ++ki)
    {
      const auto &kw = ZH_KEYWORDS[ki];
      std::string decoded_key = decode_unicode_escapes(kw.key);
      std::string_view key_view = decoded_key;

      if (i + key_view.size() > n)
        continue;

      if (memcmp(s.data() + i, key_view.data(), key_view.size()) != 0)
        continue;

      // Boundary check
      if (kw.kind == ZHK_WORD)
      {
        if (i > 0 && is_ident_char((unsigned char)s[i - 1]))
          continue;
        if (i + key_view.size() < n && is_ident_char((unsigned char)s[i + key_view.size()]))
          continue;
      }

      // Simple scoring
      float score = kw.score;
      int new_score = dp[i] + static_cast<int>(score * 100.0f);

      if (new_score > dp[i + key_view.size()])
      {
        dp[i + key_view.size()] = new_score;
        prev[i + key_view.size()] = i;
        replacement[i + key_view.size()] = kw.map_to;
        kind_history[i + key_view.size()] = static_cast<uint8_t>(kw.kind);
      }
    }

    // Skip character
    if (dp[i] > dp[i + 1])
    {
      dp[i + 1] = dp[i];
      prev[i + 1] = i;
      replacement[i + 1] = std::string(1, s[i]);
    }
  }

  // Reconstruct result
  std::string result;
  size_t pos = n;
  while (pos > 0)
  {
    if (prev[pos] == SIZE_MAX)
    {
      return s; // Error
    }
    result = replacement[pos] + result;
    pos = prev[pos];
  }

  return result;
}

// Forward declaration of the matrix-based rewriting function
std::string rewrite_with_matrix(const std::string &src);

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " <input.zh> <output.c>" << std::endl;
    std::cerr << "Translates Chinese source code to C using intelligent matrix-based keyword matching" << std::endl;
    std::cerr << "This tool reduces dependency on Python translation scripts" << std::endl;
    return 1;
  }

  const char *input_file = argv[1];
  const char *output_file = argv[2];

  // Read input file
  std::ifstream in_file(input_file, std::ios::binary);
  if (!in_file)
  {
    std::cerr << "Error: Cannot open input file '" << input_file << "'" << std::endl;
    return 1;
  }

  std::string content((std::istreambuf_iterator<char>(in_file)),
                      std::istreambuf_iterator<char>());
  in_file.close();

  // Test Unicode decoding
  auto decode_unicode_escapes = [](const std::string_view &input) -> std::string
  {
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i)
    {
      if (i + 5 < input.size() && input[i] == '\\' && input[i + 1] == 'u')
      {
        char hex[5] = {input[i + 2], input[i + 3], input[i + 4], input[i + 5], '\0'};
        unsigned int codepoint = strtoul(hex, nullptr, 16);

        if (codepoint <= 0x7F)
        {
          result.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint <= 0x7FF)
        {
          result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
          result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint <= 0xFFFF)
        {
          result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
          result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
          result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else
        {
          result.push_back('?');
        }

        i += 5;
      }
      else
      {
        result.push_back(input[i]);
      }
    }
    return result;
  };

  // Translate using matrix-based intelligent matching
  std::string translated = rewrite_with_matrix(content);
  std::ofstream out_file(output_file);
  if (!out_file)
  {
    std::cerr << "Error: Cannot open output file '" << output_file << "'" << std::endl;
    return 1;
  }

  out_file << translated;
  out_file.close();

  std::cout << "Successfully translated '" << input_file << "' to '" << output_file << "'" << std::endl;
  std::cout << "Used intelligent matrix-based keyword matching for context-aware translation" << std::endl;
  std::cout << "Reduced dependency on Python translation scripts" << std::endl;

  return 0;
}
