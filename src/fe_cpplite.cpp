#include "../include/frontend.h"
#include "../include/fe_cpplite.h"
#include <regex>
#include <map>
#include <sstream>
#include <cstdint>

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

class FE_CPPLite final : public IFrontend
{
public:
  std::string name() const override { return "cpp-lite"; }
  bool accepts(const std::string &fn, const std::string &src) const override
  {
    auto n = fn.size();
    bool ext = (n >= 4 && (fn.substr(n - 4) == ".cpp" || fn.substr(n - 3) == ".cc" || fn.substr(n - 4) == ".cxx"));
    bool has_zh = src.find(u8"輸出") != std::string::npos || src.find(u8"整數") != std::string::npos;
    return (ext || src.rfind("// cpp-lite", 0) == 0) && !has_zh;
  }
  bool compile(const FrontendContext &ctx, Bytecode &out, std::string &err) const override
  {
    std::string src = ctx.src;
    // BOM/換行正規化
    strip_utf8_bom(src);
    normalize_newlines(src);

    out.data.clear();
    std::map<std::string, uint8_t> slot; // name -> id
    auto get_slot = [&](const std::string &name) -> uint8_t
    {
      auto it = slot.find(name);
      if (it != slot.end())
        return it->second;
      uint8_t id = (uint8_t)slot.size();
      slot[name] = id;
      return id;
    };
    auto u8 = [&](unsigned v)
    { out.data.push_back((unsigned char)v); };
    auto u64 = [&](uint64_t v)
    { for(int i=0;i<8;++i) out.data.push_back((unsigned char)((v>>(i*8))&0xFF)); };
    auto i64 = [&](int64_t v)
    { u64((uint64_t)v); };
    auto s64 = [&](const std::string &s)
    { u64((uint64_t)s.size()); out.data.insert(out.data.end(), s.begin(), s.end()); };

    std::regex re_decl(R"(int\s+([A-Za-z_]\w*)\s*=\s*([0-9]+)\s*;)");
    std::regex re_cout_s(R"(std::cout\s*<<\s*\"([^\"]*)\"\s*;)");
    std::regex re_cout_id(R"(std::cout\s*<<\s*([A-Za-z_]\w*)\s*;)");

    std::stringstream ss(src);
    std::string line;
    std::smatch m;
    while (std::getline(ss, line))
    {
      if (std::regex_search(line, m, re_decl))
      {
        std::string var = m[1].str();
        int64_t val = std::stoll(m[2]);
        uint8_t id = get_slot(var);
        u8(0x03); // OP_SET_I64
        u8(id);
        i64(val);
      }
      else if (std::regex_search(line, m, re_cout_s))
      {
        std::string str = m[1].str();
        u8(0x01); // OP_PRINT
        s64(str);
      }
      else if (std::regex_search(line, m, re_cout_id))
      {
        std::string var = m[1].str();
        uint8_t id = get_slot(var);
        u8(0x02); // OP_PRINT_INT
        u8(id);
      }
      else if (line.find_first_not_of(" \t\r\n") == std::string::npos)
      {
      }
      else
      {
        err = "Unsupported C++-lite: " + line;
        return false;
      }
    }
    u8(0x04); // OP_END
    return true;
  }
};

extern "C" void register_fe_cpplite()
{
  FrontendRegistry::instance().register_frontend(std::make_unique<FE_CPPLite>());
}
static struct _Auto
{
  _Auto() { register_fe_cpplite(); }
} _autoreg;
