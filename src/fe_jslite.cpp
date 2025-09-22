#include "../include/frontend.h"
#include "../include/fe_jslite.h"
#include <regex>
#include <map>
#include <sstream>
#include <cstdint>

class FE_JSLite final : public IFrontend {
public:
  std::string name() const override { return "js-lite"; }
  bool accepts(const std::string& fn, const std::string& src) const override {
    auto n=fn.size();
    return (n>=3 && (fn.substr(n-3)==".js" || fn.substr(n-4)==".mjs")) || src.rfind("// js-lite",0)==0;
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

    std::regex re_log_s(R"(console\.log\(\s*\"([^\"]*)\"\s*\)\s*;)");
    std::regex re_let(R"(let\s+([A-Za-z_]\w*)\s*=\s*([0-9]+)\s*;)");
    std::regex re_log_id(R"(console\.log\(\s*([A-Za-z_]\w*)\s*\)\s*;)");

    std::stringstream ss(ctx.src); std::string line; std::smatch m;
    while (std::getline(ss, line)) {
      if (std::regex_search(line, m, re_log_s)) {
        std::string str = m[1].str();
        u8(0x01); // OP_PRINT
        s64(str);
      } else if (std::regex_search(line, m, re_let)) {
        std::string var = m[1].str();
        int64_t val = std::stoll(m[2]);
        uint8_t id = get_slot(var);
        u8(0x03); // OP_SET_I64
        u8(id);
        i64(val);
      } else if (std::regex_search(line, m, re_log_id)) {
        std::string var = m[1].str();
        uint8_t id = get_slot(var);
        u8(0x02); // OP_PRINT_INT
        u8(id);
      } else if (line.find_first_not_of(" \t\r\n")==std::string::npos) {
      } else { err = "Unsupported JS-lite: " + line; return false; }
    }
    u8(0x04); // OP_END
    return true;
  }
};

extern "C" void register_fe_jslite(){
  FrontendRegistry::instance().register_frontend(std::make_unique<FE_JSLite>());
}
static struct _Auto{ _Auto(){ register_fe_jslite(); } } _autoreg;