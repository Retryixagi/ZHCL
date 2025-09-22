#include "../include/frontend.h"
#include "../include/fe_zh.h"
#include "../include/zh_frontend.h"

static void emit_print (std::vector<uint8_t>& bc, const std::string& s){
  bc.push_back(0x01); // OP_PRINT
  uint64_t n = (uint64_t)s.size();
  for (int i=0;i<8;++i) bc.push_back((uint8_t)((n>>(i*8)) & 0xFF)); // << 改成 8 bytes
  bc.insert(bc.end(), s.begin(), s.end());
}
static void emit_set_i64(std::vector<uint8_t>& bc, uint8_t slot, long long v){
  bc.push_back(0x03); // OP_SET_I64
  bc.push_back(slot);
  for(int i=0;i<8;++i) bc.push_back((uint8_t)((v>>(i*8))&0xFF));
}
static void emit_print_int(std::vector<uint8_t>& bc, uint8_t slot){
  bc.push_back(0x02); // OP_PRINT_INT
  bc.push_back(slot);
}
static void emit_end    (std::vector<uint8_t>& bc){ bc.push_back(0x04); } // OP_END

class FE_ZH final : public IFrontend {
public:
  std::string name() const override { return "zh"; }
  bool accepts(const std::string& fn, const std::string& src) const override {
    auto n=fn.size(); 
    bool is_zh_ext = (n>=3 && (fn.substr(n-3)==".zh" || fn.substr(n-3)==".ZH"));
    bool is_c_with_zh = (n>=2 && fn.substr(n-2)==".c") && (src.find(u8"輸出") != std::string::npos || src.find(u8"整數") != std::string::npos);
    return is_zh_ext || is_c_with_zh;
  }
  bool compile(const FrontendContext& ctx, Bytecode& out, std::string& err) const override {
    try{
      ZhFrontend zf;
      out.data = zf.translate_to_bc(ctx.src);
      return true;
    }catch(const std::exception& e){ err = e.what(); return false; }
  }
};

extern "C" void register_fe_zh() {
  FrontendRegistry::instance().register_frontend(std::make_unique<FE_ZH>());
}

static struct _Auto{ _Auto(){ register_fe_zh(); } } _autoreg;
