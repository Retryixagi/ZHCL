#include "../include/zh_frontend.h"
#include <vector>
#include <string>
#include <cstdint>
#include <regex>
#include <map>
#include <sstream>
#include <utility>
#include <algorithm>
#include <iostream>

// Forward declaration for the new keyword rewriting function
std::string zh_keyword_rewrite(const std::string &src);

static inline void u8(std::vector<uint8_t> &bc, unsigned v) { bc.push_back((uint8_t)v); }
static inline void u64le(std::vector<uint8_t> &bc, uint64_t v)
{
    for (int i = 0; i < 8; i++)
        bc.push_back((uint8_t)((v >> (8 * i)) & 0xFF));
}
static inline void i64le(std::vector<uint8_t> &bc, int64_t v) { u64le(bc, (uint64_t)v); }

static std::string unescape_c_like(std::string s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            char c = s[++i];
            switch (c)
            {
            case 'n':
                out.push_back('\n');
                break;
            case 't':
                out.push_back('\t');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            default:
                out.push_back(c);
                break;
            }
        }
        else
            out.push_back(s[i]);
    }
    return out;
}

// Check if a character is a Chinese character (U+4E00 to U+9FFF)
// Chinese character ranges for variable names (based on tc_mapping_full.csv analysis)
// This is a single contiguous range: U+4E00 to U+9FFF (20,992 characters)
static const std::pair<char32_t, char32_t> chinese_ranges[] = {
    {0x4E00, 0x9FFF},
};
static const size_t chinese_ranges_count = 1;

// Check if a Unicode codepoint is a Chinese character (optimized range-based lookup)
static bool is_chinese_char(char32_t c)
{
    // For single range, direct comparison is fastest
    return c >= chinese_ranges[0].first && c <= chinese_ranges[0].second;

    // For multiple ranges, use binary search:
    // auto it = std::lower_bound(chinese_ranges, chinese_ranges + chinese_ranges_count,
    //                           std::make_pair(c, c),
    //                           [](const auto& p1, const auto& p2) { return p1.first < p2.first; });
    // return it != chinese_ranges + chinese_ranges_count && c <= it->second;
}

// Check if a string is a valid variable name (ASCII letters/digits/underscores or Chinese characters)
static bool is_valid_var_name(const std::string &s)
{
    if (s.empty())
        return false;

    // Convert UTF-8 string to UTF-32 for character checking
    std::u32string u32_str;
    for (size_t i = 0; i < s.size();)
    {
        char32_t cp = 0;
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80)
        {
            cp = c;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            if (i + 1 >= s.size())
                return false;
            cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F);
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            if (i + 2 >= s.size())
                return false;
            cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
            i += 3;
        }
        else
        {
            return false; // Invalid UTF-8
        }

        if (i == 1)
        { // First character
            if (!((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || cp == '_' || is_chinese_char(cp)))
                return false;
        }
        else
        { // Subsequent characters
            if (!((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || (cp >= '0' && cp <= '9') || cp == '_' || is_chinese_char(cp)))
                return false;
        }
    }
    return true;
}

// Extract variable name from a string starting at given position
static std::string extract_var_name(const std::string &s, size_t start_pos, size_t &end_pos)
{
    std::string var_name;
    size_t i = start_pos;

    while (i < s.size())
    {
        size_t char_start = i;
        char32_t cp = 0;
        unsigned char c = (unsigned char)s[i];

        if (c < 0x80)
        {
            cp = c;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            if (i + 1 >= s.size())
                break;
            cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F);
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            if (i + 2 >= s.size())
                break;
            cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
            i += 3;
        }
        else
        {
            break; // Invalid UTF-8
        }

        // Check if this character can be part of a variable name
        bool valid_char = (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') ||
                          (cp >= '0' && cp <= '9') || cp == '_' || is_chinese_char(cp);

        if (var_name.empty())
        {
            // First character must be letter, underscore, or Chinese char
            if (!((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || cp == '_' || is_chinese_char(cp)))
                break;
            var_name.append(s.substr(char_start, i - char_start));
        }
        else
        {
            // Subsequent characters
            if (!valid_char)
                break;
            var_name.append(s.substr(char_start, i - char_start));
        }
    }

    end_pos = i;
    return var_name;
}

std::vector<uint8_t> ZhFrontend::translate_to_bc(const std::string &src_in)
{
    std::string src = src_in;

    // First, apply the new intelligent keyword rewriting system
    src = zh_keyword_rewrite(src);

    std::vector<uint8_t> bc;
    std::map<std::string, uint8_t> slot;
    auto get_slot = [&](const std::string &name) -> uint8_t
    {
        auto it = slot.find(name);
        if (it != slot.end())
            return it->second;
        uint8_t id = (uint8_t)slot.size();
        slot[name] = id;
        return id;
    };

    // 三種最小語句的正則 - 現在匹配中間標記而不是原始中文
    std::regex re_print_s(u8"PRINT_STR_KEYWORD\\s*\"([^\"]*)\"");
    std::regex re_set_i64(u8"數值|數字|數字|數式"); // 兼容有需要可擴充
    // 使用更精準的「INT_KEYWORD VAR_NAME ASSIGN_KEYWORD N」
    std::regex re_set_i64_exact(u8"INT_KEYWORD\\s+");
    std::regex re_set_i64_slot(u8"INT_KEYWORD\\s+");
    std::regex re_print_i(u8"PRINT_INT_KEYWORD\\s+");
    // 添加对 C 风格的中文支持
    std::regex re_print_s_c(u8"輸出字串\\s*\\(\\s*\"([^\"]*)\"\\s*\\)\\s*;");
    std::regex re_int_assign(R"(int\s+(.+?)\s*=\s*([0-9]+)\s*;)");
    std::regex re_puts(R"(puts\s*\(\s*\"([^\"]*)\"\s*\)\s*;)");
    std::regex re_printf_d(R"(printf\s*\(\s*\"%d\"\s*,\s*([A-Za-z_]\w*)\s*\)\s*;)");

    std::stringstream ss(src);
    std::string line;
    std::smatch m;

    while (std::getline(ss, line))
    {
        // 去掉純空白行
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos)
            continue;

        // 輸出字串 - 匹配 PRINT_STR_KEYWORD "string"
        if (std::regex_search(line, m, re_print_s))
        {
            std::string str = unescape_c_like(m[1].str());
            u8(bc, 1); // OP_PRINT_S
            u64le(bc, str.size());
            for (char c : str)
                u8(bc, (unsigned char)c);
        }
        // 輸出整數 - 匹配 PRINT_INT_KEYWORD var_name
        else if (std::regex_search(line, m, re_print_i))
        {
            size_t var_start = m.position() + m.length();
            size_t var_end;
            std::string var = extract_var_name(line, var_start, var_end);
            if (!var.empty() && is_valid_var_name(var))
            {
                u8(bc, 2); // OP_PRINT_I
                u8(bc, get_slot(var));
            }
            else
            {
                // 無效變量名，跳過
                continue;
            }
        }
        // 設為數字 - 匹配 INT_KEYWORD var_name ASSIGN_KEYWORD number
        else if (std::regex_search(line, m, re_set_i64_exact))
        {
            size_t var_start = m.position() + m.length();
            size_t var_end;
            std::string var = extract_var_name(line, var_start, var_end);
            if (!var.empty() && is_valid_var_name(var))
            {
                // 查找 ASSIGN_KEYWORD 標記
                size_t assign_pos = line.find("ASSIGN_KEYWORD", var_end);
                if (assign_pos != std::string::npos)
                {
                    // 提取數字
                    size_t num_start = assign_pos + 13; // "ASSIGN_KEYWORD" 的長度
                    size_t num_end = line.find_first_not_of("0123456789-", num_start);
                    if (num_end != std::string::npos)
                    {
                        std::string num_str = line.substr(num_start, num_end - num_start);
                        try
                        {
                            int64_t val = std::stoll(num_str);
                            u8(bc, 3); // OP_SET_I64
                            u8(bc, get_slot(var));
                            i64le(bc, val);
                        }
                        catch (...)
                        {
                            // 無效數字，跳過
                            continue;
                        }
                    }
                }
            }
            else
            {
                // 無效變量名，跳過
                continue;
            }
        }
        // 設為槽位
        else if (std::regex_search(line, m, re_set_i64_slot))
        {
            size_t var_start = m.position() + m.length();
            size_t var_end;
            std::string var = extract_var_name(line, var_start, var_end);
            if (!var.empty() && is_valid_var_name(var))
            {
                // 查找 "設為槽位" 關鍵字
                size_t slot_pos = line.find(u8"設為槽位", var_end);
                if (slot_pos != std::string::npos)
                {
                    // 提取槽位號
                    size_t num_start = slot_pos + 9; // "設為槽位" 的長度
                    size_t num_end = line.find_first_not_of("0123456789", num_start);
                    if (num_end != std::string::npos)
                    {
                        std::string num_str = line.substr(num_start, num_end - num_start);
                        try
                        {
                            uint8_t slot_id = (uint8_t)std::stoi(num_str);
                            u8(bc, 4); // OP_SET_I64_SLOT
                            u8(bc, get_slot(var));
                            u8(bc, slot_id);
                        }
                        catch (...)
                        {
                            // 無效槽位號，跳過
                            continue;
                        }
                    }
                }
            }
            else
            {
                // 無效變量名，跳過
                continue;
            }
        }
        // C 風格輸出字串
        else if (std::regex_search(line, m, re_print_s_c))
        {
            std::string str = unescape_c_like(m[1].str());
            u8(bc, 1); // OP_PRINT_S
            u64le(bc, str.size());
            for (char c : str)
                u8(bc, (unsigned char)c);
        }
        // C 風格 int 賦值
        else if (std::regex_search(line, m, re_int_assign))
        {
            std::string var = m[1].str();
            int64_t val = std::stoll(m[2].str());
            u8(bc, 3); // OP_SET_I64
            u8(bc, get_slot(var));
            i64le(bc, val);
        }
        // C 風格 puts
        else if (std::regex_search(line, m, re_puts))
        {
            std::string str = unescape_c_like(m[1].str());
            u8(bc, 1); // OP_PRINT_S
            u64le(bc, str.size());
            for (char c : str)
                u8(bc, (unsigned char)c);
        }
        // C 風格 printf %d
        else if (std::regex_search(line, m, re_printf_d))
        {
            std::string var = m[1].str();
            u8(bc, 2); // OP_PRINT_I
            u8(bc, get_slot(var));
        }
        // 忽略註釋和無法識別的行
    }

    // 結束標記
    u8(bc, 0);
    return bc;
}
