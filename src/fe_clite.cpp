#include "../include/frontend.h"
#include "../include/fe_clite.h"
#include <regex>
#include <map>
#include <sstream>
#include <cstdint>

class FE_CLite final : public IFrontend {
public:
  std::string name() const override { return "c-lite"; }
  bool accepts(const std::string& fn, const std::string& src) const override {
    auto n = fn.size();
    bool is_c = (n>=2 && (fn.substr(n-2)==".c"));
    bool has_zh = src.find(u8"輸出") != std::string::npos || src.find(u8"整數") != std::string::npos;
    return is_c && !has_zh;
  }
  bool compile(const FrontendContext& ctx, Bytecode& out, std::string& err) const override {
    out.data.clear();
    std::map<std::string,uint8_t> slot; // name -> id
    auto get_slot = [&](const std::string& name)->uint8_t{
      auto it = slot.find(name);
      if(it!=slot.end()) return it->second;
      uint8_t id = (uint8_t)slot.size();
      slot[name] = id;
      return id;
    };
    auto u8  = [&](unsigned v){ out.data.push_back((unsigned char)v); };
    auto u64 = [&](uint64_t v){ for(int i=0;i<8;++i) out.data.push_back((unsigned char)((v>>(i*8))&0xFF)); };
    auto i64 = [&](int64_t v){ u64((uint64_t)v); };
    auto s64 = [&](const std::string& s){ u64((uint64_t)s.size()); out.data.insert(out.data.end(), s.begin(), s.end()); };

    std::regex re_decl(R"(int\s+([A-Za-z_]\w*)\s*=\s*([0-9]+)\s*;)");
    std::regex re_puts(R"(puts\s*\(\s*\"([^\"]*)\"\s*\)\s*;)");
    std::regex re_printf_s(R"(printf\s*\(\s*\"([^\"]*)\"\s*\)\s*;)");
    std::regex re_printf_d(R"(printf\s*\(\s*\"%d\"\s*,\s*([A-Za-z_]\w*)\s*\)\s*;)");

    std::stringstream ss(ctx.src); std::string line; std::smatch m;
    while (std::getline(ss, line)) {
      if (std::regex_search(line, m, re_decl)) {
        std::string var = m[1].str();
        int64_t val = std::stoll(m[2]);
        uint8_t id = get_slot(var);
        u8(0x03); // OP_SET_I64
        u8(id);
        i64(val);
      } else if (std::regex_search(line, m, re_puts) || std::regex_search(line, m, re_printf_s)) {
        std::string str = m[1].str();
        u8(0x01); // OP_PRINT
        s64(str);
      } else if (std::regex_search(line, m, re_printf_d)) {
        std::string var = m[1].str();
        uint8_t id = get_slot(var);
        u8(0x02); // OP_PRINT_INT
        u8(id);
      } else if (line.find_first_not_of(" \t\r\n")==std::string::npos) {
        // skip
      } else { err = "Unsupported C-lite: " + line; return false; }
    }
    u8(0x04); // OP_END
    return true;
  }
};

extern "C" void register_fe_clite(){
  FrontendRegistry::instance().register_frontend(std::make_unique<FE_CLite>());
}
static struct _Auto{ _Auto(){ register_fe_clite(); } } _autoreg;