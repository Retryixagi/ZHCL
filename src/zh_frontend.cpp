#include "../include/zh_frontend.h"
#include <vector>
#include <string>
#include <cstdint>
#include <regex>
#include <map>
#include <sstream>

static inline void u8(std::vector<uint8_t>& bc, unsigned v){ bc.push_back((uint8_t)v); }
static inline void u64le(std::vector<uint8_t>& bc, uint64_t v){ for(int i=0;i<8;i++) bc.push_back((uint8_t)((v>>(8*i))&0xFF)); }
static inline void i64le(std::vector<uint8_t>& bc, int64_t v){ u64le(bc, (uint64_t)v); }

static std::string unescape_c_like(std::string s){
  std::string out; out.reserve(s.size());
  for (size_t i=0;i<s.size();++i){
    if (s[i]=='\\' && i+1<s.size()){
      char c=s[++i];
      switch(c){
        case 'n': out.push_back('\n'); break;
        case 't': out.push_back('\t'); break;
        case 'r': out.push_back('\r'); break;
        case '\\': out.push_back('\\'); break;
        case '"': out.push_back('"'); break;
        default: out.push_back('\\'); out.push_back(c); break;
      }
    } else out.push_back(s[i]);
  }
  return out;
}

std::vector<uint8_t> ZhFrontend::translate_to_bc(const std::string& src_in) {
    std::string src = src_in;

    std::vector<uint8_t> bc;
    std::map<std::string,uint8_t> slot;
    auto get_slot = [&](const std::string& name)->uint8_t{
        auto it = slot.find(name);
        if(it!=slot.end()) return it->second;
        uint8_t id = (uint8_t)slot.size();
        slot[name] = id;
        return id;
    };

    // 三種最小語句的正則
    std::regex re_print_s(u8"\u8F38\u51FA\\s*\u5B57\u4E32\\s*\"([^\"]*)\"");
    std::regex re_set_i64(u8"\u6578\u503C|\u6578\u5B57|\u6578\u5B50|\u6578\u5F0F"); // 兼容有需要可擴充
    // 使用更精準的「整數 X 設為 N」
    std::regex re_set_i64_exact(u8"\u6574\u6578\\s+([\\p{L}_][\\p{L}0-9_]*)\\s*\u8A2D\u70BA\\s*(-?[0-9]+)");
    std::regex re_print_i(u8"\u8F38\u51FA\\s*\u6574\u6578\\s+([\\p{L}_][\\p{L}0-9_]*)");
    // 添加对 C 风格的中文支持
    std::regex re_print_s_c(u8"輸出字串\\s*\\(\\s*\"([^\"]*)\"\\s*\\)\\s*;");
    std::regex re_int_assign(R"(int\s+([A-Za-z_]\w*)\s*=\s*([0-9]+)\s*;)");
    std::regex re_puts(R"(puts\s*\(\s*\"([^\"]*)\"\s*\)\s*;)");
    std::regex re_printf_d(R"(printf\s*\(\s*\"%d\"\s*,\s*([A-Za-z_]\w*)\s*\)\s*;)");

    std::stringstream ss(src);
    std::string line; std::smatch m;

    while (std::getline(ss, line)) {
        // 去掉純空白行
        auto pos = line.find_first_not_of(" \t\r\n");
        if (pos == std::string::npos) continue;

        // 輸出 字串 "..."
        if (std::regex_search(line, m, re_print_s)) {
            std::string s = m[1].str();
            s = unescape_c_like(s);
            u8(bc, 0x01);                  // OP_PRINT
            u64le(bc, (uint64_t)s.size());  // u64 長度
            bc.insert(bc.end(), s.begin(), s.end());
            continue;
        }

        // 整數 <變數> 設為 <值>
        if (std::regex_search(line, m, re_set_i64_exact)) {
            std::string var = m[1].str();
            int64_t val = std::stoll(m[2].str());
            uint8_t id = get_slot(var);
            u8(bc, 0x03);  // OP_SET_I64
            u8(bc, id);    // 槽位
            i64le(bc, val);
            continue;
        }

        // 輸出 整數 <變數>
        if (std::regex_search(line, m, re_print_i)) {
            std::string var = m[1].str();
            uint8_t id = get_slot(var);
            u8(bc, 0x02);  // OP_PRINT_INT
            u8(bc, id);
            continue;
        }

        // C 风格中文支持
        if (std::regex_search(line, m, re_print_s_c)) {
            std::string str = m[1].str();
            str = unescape_c_like(str);
            u8(bc, 0x01); // OP_PRINT
            uint64_t n = (uint64_t)str.size();
            for (int i=0;i<8;++i) bc.push_back((uint8_t)((n>>(i*8)) & 0xFF));
            bc.insert(bc.end(), str.begin(), str.end());
            continue;
        }
        if (std::regex_search(line, m, re_int_assign)) {
            std::string var = m[1].str();
            long long val = std::stoll(m[2]);
            uint8_t slot = get_slot(var);
            u8(bc, 0x03); // OP_SET_I64
            bc.push_back(slot);
            for(int i=0;i<8;++i) bc.push_back((uint8_t)((val>>(i*8))&0xFF));
            continue;
        }
        if (std::regex_search(line, m, re_puts)) {
            std::string str = m[1].str();
            str = unescape_c_like(str);
            u8(bc, 0x01); // OP_PRINT
            uint64_t n = (uint64_t)str.size();
            for (int i=0;i<8;++i) bc.push_back((uint8_t)((n>>(i*8)) & 0xFF));
            bc.insert(bc.end(), str.begin(), str.end());
            continue;
        }
        if (std::regex_search(line, m, re_printf_d)) {
            std::string var = m[1].str();
            uint8_t slot = get_slot(var);
            u8(bc, 0x02); // OP_PRINT_INT
            bc.push_back(slot);
            continue;
        }

        // 其餘先忽略（需要再擴充）
    }

    u8(bc, 0x04); // OP_END
    return bc;
}