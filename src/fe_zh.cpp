#include "../include/frontend.h"
#include "../include/fe_zh.h"
#include "../include/zh_frontend.h"
#include "../include/chinese.h"
#include "../include/zh_matrix.inc" // Matrix configuration for intelligent keyword matching
#include <regex>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>
#include <fstream>
#include <vector>
#include <cstdio>
#include <iostream>

// Keyword table is now included via chinese.h

// Global storage for loaded keywords
static std::vector<std::string> loaded_keys;
static std::vector<std::string> loaded_maps;
static std::vector<ZhKeyword> loaded_keywords;
static bool keywords_loaded = false;

// Load keywords from CSV file
static void load_keywords_from_csv()
{
  if (keywords_loaded)
    return;

  loaded_keywords.clear();
  loaded_keys.clear();
  loaded_maps.clear();

  std::ifstream file("zh_keywords.csv");
  if (!file.is_open())
  {
    // Fallback to hardcoded keywords if CSV not found
    loaded_keywords.assign(ZH_KEYWORDS, ZH_KEYWORDS + ZH_KEYWORDS_COUNT);
    keywords_loaded = true;
    return;
  }

  std::string line;
  // Skip header
  std::getline(file, line);

  while (std::getline(file, line))
  {
    if (line.empty())
      continue;

    size_t comma_pos = line.find(',');
    if (comma_pos == std::string::npos)
      continue;

    std::string phrase = line.substr(0, comma_pos);
    std::string token = line.substr(comma_pos + 1);

    // Store strings and create string_views
    loaded_keys.push_back(phrase);
    loaded_maps.push_back(token);

    ZhKeyword kw;
    kw.key = loaded_keys.back();
    kw.map_to = loaded_maps.back();
    kw.kind = ZHK_WORD;
    kw.score = 1.0f;
    kw.tags = "";

    loaded_keywords.push_back(kw);
  }

  keywords_loaded = true;
}

// Provide access to the keyword table
const ZhKeyword *get_zh_keywords()
{
  load_keywords_from_csv();
  return loaded_keywords.data();
}

size_t get_zh_keywords_count()
{
  load_keywords_from_csv();
  return loaded_keywords.size();
}

// Matrix-based rewriting with string protection and boundary checking
std::string rewrite_with_matrix(const std::string &src)
{
  std::string s = src;

  // Preprocessing
  zh_strip_utf8_bom(s.data(), s.size());
  zh_normalize_newlines(s);
  zh_nfkc(s);
  // zh_simplify(s); // Disabled: keep traditional Chinese as requested

  std::cout << "After preprocessing: '" << s << "' (len=" << s.size() << ")" << std::endl;

  size_t n = s.size();
  std::vector<int> dp(n + 1, INT_MIN);             // Best score ending at position i
  std::vector<size_t> prev(n + 1, SIZE_MAX);       // Previous position for reconstruction
  std::vector<std::string> replacement(n + 1, ""); // Replacement string for segment
  std::vector<uint8_t> kind_history(n + 1, 0);     // Track token kinds for matrix scoring

  dp[0] = 0;           // Start with score 0
  kind_history[0] = 0; // Start with WORD kind

  // Initialize: allow skipping all characters as fallback
  for (size_t j = 1; j <= n; ++j)
  {
    if (dp[j - 1] > dp[j])
    {
      dp[j] = dp[j - 1];
      prev[j] = j - 1;
      replacement[j] = std::string(1, s[j - 1]);
    }
  }

  // Helper: check if character is identifier character
  auto is_ident_char = [](unsigned char c) -> bool
  {
    return isalnum(c) || c == '_' || (c >= 128); // Include Unicode for Chinese
  };

  // Helper: determine token kind from keyword
  auto get_token_kind = [](const ZhKeyword &kw) -> uint8_t
  {
    return static_cast<uint8_t>(kw.kind);
  };

  // Build the DP table
  for (size_t i = 0; i < n; ++i)
  {
    if (dp[i] == INT_MIN)
      continue; // Unreachable position

    // Check if we're inside a string literal at position i
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
      continue; // Skip keyword matching inside strings

    // Try all keywords at this position
    const auto *keywords = get_zh_keywords();
    size_t count = get_zh_keywords_count();

    for (size_t ki = 0; ki < count; ++ki)
    {
      const auto &kw = keywords[ki];
      std::string_view key_view = kw.key;  // Use UTF-8 directly
      std::string_view val_view = kw.map_to;  // Use string_view directly
      float score = kw.score;

      // Debug output
      if (ki < 5)
      { // Only print first few for debugging
        std::cout << "Keyword " << ki << ": '" << kw.key << "' -> '" << kw.map_to << "' (len=" << key_view.size() << ")" << std::endl;
      }

      if (i + key_view.size() > n)
        continue;

      // Check if the keyword matches
      if (memcmp(s.data() + i, key_view.data(), key_view.size()) != 0)
        continue;

      // Boundary check for WORD keywords
      if (kw.kind == ZHK_WORD)
      {
        if (i > 0 && is_ident_char((unsigned char)s[i - 1]))
          continue;
        if (i + key_view.size() < n && is_ident_char((unsigned char)s[i + key_view.size()]))
          continue;
      }

      // Apply matrix-based scoring with context awareness
      uint8_t current_kind = kind_history[i];
      uint8_t keyword_kind = get_token_kind(kw);

      // Matrix transition weight
      float matrix_weight = ZH_KIND_TRANS[current_kind][keyword_kind];

      // Length bonus
      float length_bonus = key_view.size() * ZH_LEN_BONUS;

      // Tag matching bonus (simplified - could be enhanced)
      float tag_bonus = 0.0f;
      // TODO: Implement tag matching logic if needed

      // Calculate final score with matrix weights
      float final_score = score * matrix_weight + length_bonus + tag_bonus;

      // Update DP if this gives a better score
      size_t next_pos = i + key_view.size();
      int new_score = dp[i] + static_cast<int>(final_score * 100.0f);

      if (new_score > dp[next_pos])
      {
        dp[next_pos] = new_score;
        prev[next_pos] = i;
        replacement[next_pos] = std::string(val_view);
        kind_history[next_pos] = keyword_kind; // Track kind for next transitions
      }
    }

    // Also consider skipping this character (score 0)
    if (dp[i] > dp[i + 1])
    {
      dp[i + 1] = dp[i];
      prev[i + 1] = i;
      replacement[i + 1] = std::string(1, s[i]);
    }
  }

  // Reconstruct the result
  std::string result;
  size_t pos = n;
  while (pos > 0)
  {
    if (prev[pos] == SIZE_MAX)
    {
      // If we can't reconstruct, return original string
      return s;
    }
    result = replacement[pos] + result;
    pos = prev[pos];
  }

  return result;
}

// 共用小工具：BOM/換行正規化
static inline void strip_utf8_bom(std::string &s)
{
  if (s.size() >= 3 &&
      (unsigned char)s[0] == 0xEF &&
      (unsigned char)s[1] == 0xBB &&
      (unsigned char)s[2] == 0xBF)
  {
    s.erase(0, 3);
  }
}

static inline void normalize_newlines(std::string &s)
{
  // 把 CRLF/CR 全部正規化成 \n
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i)
  {
    char c = s[i];
    if (c == '\r')
    {
      if (i + 1 < s.size() && s[i + 1] == '\n')
        ++i; // 吃掉 \r\n 的 \n
      out.push_back('\n');
    }
    else
    {
      out.push_back(c);
    }
  }
  s.swap(out);
}

// Chinese text processing utilities (implementing chinese.h)

// Strip UTF-8 BOM from string
void zh_strip_utf8_bom(char *s, size_t len)
{
  if (len >= 3 &&
      (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF)
  {
    // For char* version, we can't modify the string in place easily
    // Just skip BOM by shifting the string
    memmove(s, s + 3, len - 3);
    s[len - 3] = '\0';
  }
}

// Normalize newlines (CRLF -> LF, CR -> LF)
void zh_normalize_newlines(std::string &s)
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

// Unicode NFKC normalization (stub for future implementation)
void zh_nfkc(std::string &s)
{
  // TODO: Implement Unicode NFKC normalization if needed
  // For now, leave as-is
}

// Traditional to Simplified Chinese conversion (stub for future implementation)
void zh_simplify(std::string &s)
{
  // TODO: Implement traditional to simplified conversion using mapping table
  // For now, leave as-is
}

// Check if character can be part of an identifier (alphanumeric, underscore, or non-ASCII)
static inline bool is_ident_char(unsigned char c)
{
  // ASCII alphanumeric or underscore
  if (std::isalnum(c) || c == '_')
    return true;
  // Non-ASCII characters (including Chinese) are considered identifier chars
  return (c & 0x80) != 0;
}

// Main keyword rewriting function: replace Chinese keywords with tokens
// Uses longest-match replacement while preserving strings and identifiers
std::string zh_keyword_rewrite(const std::string &src)
{
  // Use the new matrix-based rewriting with string protection and boundary checking
  std::string result = rewrite_with_matrix(src);
  // Temporary debug output
  std::cout << "[ZH_REWRITE] Input: " << src << std::endl;
  std::cout << "[ZH_REWRITE] Output: " << result << std::endl;
  return result;
}

static void emit_print(std::vector<uint8_t> &bc, const std::string &s)
{
  bc.push_back(0x01); // OP_PRINT
  uint64_t n = (uint64_t)s.size();
  for (int i = 0; i < 8; ++i)
    bc.push_back((uint8_t)((n >> (i * 8)) & 0xFF)); // << 改成 8 bytes
  bc.insert(bc.end(), s.begin(), s.end());
}
static void emit_set_i64(std::vector<uint8_t> &bc, uint8_t slot, long long v)
{
  bc.push_back(0x03); // OP_SET_I64
  bc.push_back(slot);
  for (int i = 0; i < 8; ++i)
    bc.push_back((uint8_t)((v >> (i * 8)) & 0xFF));
}
static void emit_copy_i64(std::vector<uint8_t> &bc, uint8_t dst, uint8_t src)
{
  bc.push_back(0x06); // OP_COPY_I64
  bc.push_back(dst);
  bc.push_back(src);
}
static void emit_end(std::vector<uint8_t> &bc) { bc.push_back(0x04); } // OP_END

class FE_ZH final : public IFrontend
{
public:
  std::string name() const override { return "zh"; }
  bool accepts(const std::string &fn, const std::string &src) const override
  {
    auto n = fn.size();
    bool is_zh_ext = (n >= 3 && (fn.substr(n - 3) == ".zh" || fn.substr(n - 3) == ".ZH"));
    bool is_c_with_zh = (n >= 2 && fn.substr(n - 2) == ".c") && (src.find(u8"輸出") != std::string::npos || src.find(u8"整數") != std::string::npos);
    return is_zh_ext || is_c_with_zh;
  }
  bool compile(const FrontendContext &ctx, Bytecode &out, std::string &err) const override
  {
    try
    {
      std::string src = ctx.src;
      // Use the new keyword rewriting system
      std::string normalized = zh_keyword_rewrite(src);

      ZhFrontend zf;
      out.data = zf.translate_to_bc(normalized);
      return true;
    }
    catch (const std::exception &e)
    {
      err = e.what();
      return false;
    }
  }
};

extern "C" void register_fe_zh()
{
  FrontendRegistry::instance().register_frontend(std::make_unique<FE_ZH>());
}

static struct _Auto
{
  _Auto() { register_fe_zh(); }
} _autoreg;
