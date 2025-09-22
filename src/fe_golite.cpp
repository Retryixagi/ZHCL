#include "../include/fe_golite.h"
#include "../include/frontend.h"
#include <regex>
#include <map>
#include <sstream>

class FE_GoLite final : public IFrontend {
public:
    std::string name() const override { return "go-lite"; }
    bool accepts(const std::string& path, const std::string& src) const override;
    bool compile(const FrontendContext& ctx, Bytecode& out, std::string& err) const override;
};

static void u8(std::vector<uint8_t>& v, uint8_t x){ v.push_back(x); }
static void u64le(std::vector<uint8_t>& v, uint64_t x){ for(int i=0;i<8;i++) v.push_back((uint8_t)(x>>(8*i))); }
static void i64le(std::vector<uint8_t>& v, int64_t x){ u64le(v,(uint64_t)x); }
static void emit_print(Bytecode& bc, const std::string& s){
    u8(bc.data, 0x01); u64le(bc.data, (uint64_t)s.size()); bc.data.insert(bc.data.end(), s.begin(), s.end());
}
static void emit_set_i64(Bytecode& bc, uint8_t slot, int64_t v){
    u8(bc.data, 0x03); u8(bc.data, slot); i64le(bc.data, v);
}
static void emit_print_int(Bytecode& bc, uint8_t slot){
    u8(bc.data, 0x02); u8(bc.data, slot);
}

bool FE_GoLite::accepts(const std::string& path, const std::string& src) const {
    auto ends = [&](const char* ext){ return path.size()>=3 && path.rfind(ext)==path.size()-strlen(ext); };
    if (ends(".go")) return true;
    // 內容嗅探：有 package / import / fmt.Println( 或 := )
    if (src.find("fmt.Println(")!=std::string::npos) return true;
    if (src.find("package ")!=std::string::npos) return true;
    return false;
}

bool FE_GoLite::compile(const FrontendContext& ctx, Bytecode& out, std::string& err) const {
    std::map<std::string,uint8_t> slot;
    auto slot_of = [&](const std::string& name)->uint8_t{
        auto it=slot.find(name); if(it!=slot.end()) return it->second;
        uint8_t id=(uint8_t)slot.size(); slot[name]=id; return id;
    };
    std::regex re_print_s(R"(fmt\.Println\(\s*\"([^\"]*)\"\s*\)\s*;?)");
    std::regex re_set_i(R"((?:var\s+)?([A-Za-z_]\w*)\s*(?::=\s*|\s*=\s*|\s+int\s*=\s*)(-?\d+)\s*;?)");
    std::regex re_print_i(R"(fmt\.Println\(\s*([A-Za-z_]\w*)\s*\)\s*;?)");

    std::string line;
    std::stringstream ss(ctx.src);
    while (std::getline(ss, line)) {
        std::smatch m;
        if (std::regex_search(line, m, re_print_s)) { emit_print(out, m[1].str()); continue; }
        if (std::regex_search(line, m, re_set_i))   { auto id=slot_of(m[1].str()); emit_set_i64(out,id, std::stoll(m[2].str())); continue; }
        if (std::regex_search(line, m, re_print_i)) { auto id=slot_of(m[1].str()); emit_print_int(out,id); continue; }
    }
    u8(out.data, 0x04);
    return true;
}

extern "C" void register_fe_golite(){
    FrontendRegistry::instance().register_frontend(std::make_unique<FE_GoLite>());
}