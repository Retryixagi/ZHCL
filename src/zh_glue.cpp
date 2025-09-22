// zh_glue.cpp  — 將獨立的繁中前端接回 zhcl_universal
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include "zh_frontend.h"

// 這些 OP_* 與位元碼佈局必須與 zhcl_universal.cpp 的 execute_bc()/disassemble_bc() 一致
// 如果你的 enum 在別處定義，這段 include/宣告要對齊。
#ifndef OP_PRINT
enum OpCode : uint8_t {
    OP_PRINT      = 1,
    OP_PRINT_INT  = 2,
    OP_SET_I64    = 3,
    OP_END        = 4,
};
#endif

static inline void emit_u64(std::vector<uint8_t>& bc, uint64_t v){
    for(int i=0;i<8;i++) bc.push_back((uint8_t)((v>>(8*i))&0xFF)); // little-endian
}
static inline void emit_i64(std::vector<uint8_t>& bc, int64_t v){
    emit_u64(bc, (uint64_t)v);
}

static void emit_PRINT(std::vector<uint8_t>& bc, const std::string& s){
    bc.push_back((uint8_t)OP_PRINT);
    emit_u64(bc, (uint64_t)s.size());
    bc.insert(bc.end(), s.begin(), s.end());
}
static void emit_SET_I64(std::vector<uint8_t>& bc, uint8_t slot, long long v){
    bc.push_back((uint8_t)OP_SET_I64);
    bc.push_back(slot);
    emit_i64(bc, v);
}
static void emit_PRINT_INT(std::vector<uint8_t>& bc, uint8_t slot){
    bc.push_back((uint8_t)OP_PRINT_INT);
    bc.push_back(slot);
}
static void emit_END(std::vector<uint8_t>& bc){
    bc.push_back((uint8_t)OP_END);
}

// 由主程式提供（已存在於 zhcl_universal.cpp）
namespace selfhost {
    extern std::string emit_cpp_from_bc(const std::vector<uint8_t>& bc);
}

// 給 ChineseProcessor::compile 使用：.zh → .cpp（完全不需要 Python）
extern "C" int translate_zh_to_cpp(const std::string& input_path,
                                   const std::string& output_cpp_path,
                                   bool verbose)
{
    // 讀入 .zh 原始碼
    std::ifstream ifs(input_path, std::ios::binary);
    if(!ifs){
        if(verbose) std::fprintf(stderr, "[zhcl] cannot open %s\n", input_path.c_str());
        return 1;
    }
    std::string src((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    // .zh -> bytecode（繁體限定；觸發簡體關鍵詞會丟例外）
    std::vector<uint8_t> bc;
    try{
        ZhFrontend fe;
        bc = fe.translate_to_bc(src);
    } catch(const std::exception& e){
        std::fprintf(stderr, "[zhcl] zh->bc failed: %s\n", e.what());
        return 2;
    }

    // bytecode -> C++ 原始碼
    std::string cpp = selfhost::emit_cpp_from_bc(bc);
    std::ofstream ofs(output_cpp_path, std::ios::binary);
    if(!ofs){
        if(verbose) std::fprintf(stderr, "[zhcl] cannot write %s\n", output_cpp_path.c_str());
        return 3;
    }
    ofs.write(cpp.data(), (std::streamsize)cpp.size());
    if(verbose) std::printf("[zhcl] emitted C++ -> %s\n", output_cpp_path.c_str());
    return 0;
}
