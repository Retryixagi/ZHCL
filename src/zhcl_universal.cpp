// zhcl.cpp -- Universal Compiler & Build System Replacement
// One CPP file to rule them all: C, C++, Java, Python, Go, Rust, JS/TS, and more
// Replaces cl, gcc, g++, javac, cmake, make, and build systems

#include "../include/chinese.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <map>
#include <set>
#include <filesystem>
#include <regex>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

// Use explicit std:: prefix instead of using namespace std
namespace fs = std::filesystem;
using namespace std;
#include <thread>
#include <mutex>
#include <atomic>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// ======= SelfHost: payload trailer + runtime + packer (no external compilers) =======
#ifdef _WIN32
#include <windows.h>
#endif
#include <cstdint>
#include <fstream>
#include <sstream>

namespace selfhost {

// ---- New: CRC32 (簡單、足夠) ----
static uint32_t crc32(const uint8_t* data, size_t len){
    uint32_t c = 0xFFFFFFFFu;
    for(size_t i=0;i<len;i++){
        c ^= data[i];
        for(int k=0;k<8;k++){
            uint32_t m = -(int)(c & 1u);
            c = (c >> 1) ^ (0xEDB88320u & m);
        }
    }
    return ~c;
}

static const uint64_t SH_MAGIC = 0x305941505A48435Full; // 同原本
static const uint32_t SH_VERSION = 1;                   // <== New: 版本

#pragma pack(push,1)
struct Trailer {
    uint64_t magic;          // SH_MAGIC
    uint64_t payload_size;   // 位元碼長度
    uint64_t payload_offset; // 位元碼起點
    uint32_t version;        // <== New
    uint32_t crc32;          // <== New (對 payload 計算)
};
#pragma pack(pop)

enum Op : uint8_t { OP_PRINT = 1 };

// ---- 前向聲明 ----
static void disassemble_bc(const std::vector<uint8_t>& bc, std::ostream& out);
static int handle_selfhost_explain(int argc, char** argv);

// ---- 小工具 ----
static std::string read_all(const std::filesystem::path& p){
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss; ss<<f.rdbuf(); return ss.str();
}
static bool write_all(const std::filesystem::path& p, const std::string& s){
    std::filesystem::create_directories(p.parent_path()); std::ofstream f(p, std::ios::binary);
    f.write(s.data(), (std::streamsize)s.size()); return (bool)f;
}
static bool file_copy(const std::filesystem::path& src, const std::filesystem::path& dst){
    std::ifstream in(src, std::ios::binary); if(!in) return false;
    std::ofstream out(dst, std::ios::binary); if(!out) return false;
    out<<in.rdbuf(); return (bool)out;
}
static std::vector<uint8_t> enc_print(const std::string& s){
    std::vector<uint8_t> out; uint64_t n = (uint64_t)s.size();
    out.push_back((uint8_t)OP_PRINT);
    for(int i=0;i<8;i++) out.push_back((uint8_t)((n>>(8*i))&0xFF)); // u64 LE
    out.insert(out.end(), s.begin(), s.end());
    return out;
}

// ---- 直譯器（執行位元碼）----
static void execute_bc(const std::vector<uint8_t>& bc){
#ifdef _WIN32
    // 設置控制台代碼頁為 UTF-8 以正確顯示中文
    SetConsoleOutputCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD w=0;
    size_t i=0;
    while(i<bc.size()){
        uint8_t op = bc[i++];
        if(op==(uint8_t)OP_PRINT){
            if(i+8>bc.size()) break;
            uint64_t n=0; for(int k=0;k<8;k++) n|=((uint64_t)bc[i++])<<(8*k);
            if(i+n>bc.size()) break;
            const char* s=(const char*)&bc[i];
            WriteFile(hOut, s, (DWORD)n, &w, nullptr);
            WriteFile(hOut, "\r\n", 2, &w, nullptr);
            i += (size_t)n;
        }else break;
    }
    ExitProcess(0); // 直接結束，不回 CLI
#else
    // POSIX 環境也可用 fwrite/puts 版本（如需）
    size_t i=0;
    while(i<bc.size()){
        uint8_t op = bc[i++];
        if(op==(uint8_t)OP_PRINT){
            if(i+8>bc.size()) break;
            uint64_t n=0; for(int k=0;k<8;k++) n|=((uint64_t)bc[i++])<<(8*k);
            if(i+n>bc.size()) break;
            fwrite(&bc[i],1,(size_t)n,stdout); fputc('\n',stdout);
            i += (size_t)n;
        }else break;
    }
    std::exit(0);
#endif
}

// ---- 反組譯位元碼為可讀格式 ----
static void disassemble_bc(const std::vector<uint8_t>& bc, std::ostream& out = std::cout) {
    out << "Bytecode disassembly:" << std::endl;
    size_t i = 0;
    while (i < bc.size()) {
        out << std::setw(4) << std::setfill('0') << i << ": ";
        uint8_t op = bc[i++];
        switch (op) {
            case (uint8_t)OP_PRINT: {
                if (i + 8 > bc.size()) {
                    out << "PRINT (incomplete)" << std::endl;
                    return;
                }
                uint64_t n = 0;
                for (int k = 0; k < 8; k++) n |= ((uint64_t)bc[i++]) << (8 * k);
                if (i + n > bc.size()) {
                    out << "PRINT (incomplete string)" << std::endl;
                    return;
                }
                out << "PRINT \"";
                for (size_t j = 0; j < n; j++) {
                    char c = (char)bc[i + j];
                    if (c == '\n') out << "\\n";
                    else if (c == '\r') out << "\\r";
                    else if (c == '\t') out << "\\t";
                    else if (c == '"') out << "\\\"";
                    else if (c == '\\') out << "\\\\";
                    else if (c >= 32 && c <= 126) out << c;
                    else out << "\\x" << std::setw(2) << std::setfill('0') << std::hex << (int)(unsigned char)c << std::dec;
                }
                out << "\"" << std::endl;
                i += (size_t)n;
                break;
            }
            default:
                out << "UNKNOWN_OP(0x" << std::setw(2) << std::setfill('0') << std::hex << (int)op << std::dec << ")" << std::endl;
                return;
        }
    }
    out << "End of bytecode" << std::endl;
}

// ---- New: 讀取 + 驗證 trailer，回傳 payload 與資訊 ----
struct PayloadInfo {
    std::vector<uint8_t> data;
    Trailer tr{};
    bool ok=false;
    bool crc_ok=false;
};
static PayloadInfo read_payload_from_file(const std::filesystem::path& exe){
    PayloadInfo R;
    std::ifstream f(exe, std::ios::binary); if(!f) return R;
    f.seekg(0, std::ios::end);
    auto sz = (uint64_t)f.tellg();
    if(sz < sizeof(Trailer)) return R;
    f.seekg(sz - sizeof(Trailer));
    f.read((char*)&R.tr, sizeof(Trailer));
    if(!f || R.tr.magic != SH_MAGIC) return R;
    R.ok = true;
    R.data.resize((size_t)R.tr.payload_size);
    f.seekg((std::streamoff)R.tr.payload_offset);
    f.read((char*)R.data.data(), (std::streamsize)R.tr.payload_size);
    if(!f) { R.ok=false; return R; }
    uint32_t c = crc32(R.data.data(), R.data.size());
    R.crc_ok = (c == R.tr.crc32);
    return R;
}

// ---- runtime：啟動時若帶 payload 就印自證橫幅 -> 執行 -> 退出 ----
static bool maybe_run_embedded_payload(){
#ifdef _WIN32
    wchar_t pathW[MAX_PATH]{0};
    GetModuleFileNameW(nullptr, pathW, MAX_PATH);
    std::filesystem::path self(pathW);
#else
    std::filesystem::path self = "/proc/self/exe";
#endif
    auto R = read_payload_from_file(self);
    if(!R.ok) return false;

    // 讀到 R 之後、印橫幅之前加入：
    bool show_proof = false;

    // 1) 命令列參數觸發（支援 --prove / --proof / --selfhost-info）
    #ifdef _WIN32
    {
        std::wstring cmd = GetCommandLineW();
        std::wstring lower = cmd;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
        if (lower.find(L"--prove") != std::wstring::npos ||
            lower.find(L"--proof") != std::wstring::npos ||
            lower.find(L"--selfhost-info") != std::wstring::npos) {
            show_proof = true;
        }
    }
    #else
    {
        // 非 Windows 可用 argc/argv 或 /proc/self/cmdline（依你的現有架構擇一）
        // 下面是範例：若你在這個早期路徑拿不到 argv，可以先略過（僅用環境變數啟用）
    }
    #endif

    // 2) 環境變數觸發（CI/腳本用）
    if (const char* s = std::getenv("ZHCL_SELFHOST_SHOW")) {
        if (s[0]=='1' || s[0]=='y' || s[0]=='Y' || s[0]=='t' || s[0]=='T') {
            show_proof = true;
        }
    }

    // 3) 若你有在 Trailer.flags 設過「SHF_SILENT_BANNER」，此處可完全忽略它
    //    因為我們現在預設就是靜默，只有顯式要求才顯示。

    // —— 原本這裡是直接 printf 橫幅 ——
    //    改成：只有 show_proof 才印
    if (show_proof) {
        std::printf("[selfhost] payload v%u %s size=%llu crc=%s\n",
            R.tr.version,
            (R.ok?"found":"missing"),
            (unsigned long long)R.tr.payload_size,
            (R.crc_ok?"OK":"BAD"));
    }

    if(!R.crc_ok){
        std::fprintf(stderr, "[selfhost] CRC mismatch, abort.\n");
        std::exit(3);
    }

    execute_bc(R.data); // 不返回
    return true;
}

// ---- 翻譯器：JS → 位元碼（PoC：console.log("…") / 其他忽略）----
static std::vector<uint8_t> translate_js_to_bc(const std::string& js){
    std::vector<uint8_t> bc; std::istringstream ss(js); std::string line;
    // 處理按行分割的輸入
    while(std::getline(ss,line)){
        // 去除前後空白
        size_t start = line.find_first_not_of(" \t");
        if(start == std::string::npos) continue;
        line = line.substr(start);
        if(line.empty()) continue;

        auto pos = line.find("console.log");
        if(pos != std::string::npos){
            // 找到 console.log( 之後的內容
            size_t paren_start = line.find('(', pos);
            if(paren_start == std::string::npos) continue;
            size_t paren_end = line.find(')', paren_start);
            if(paren_end == std::string::npos) continue;

            std::string arg = line.substr(paren_start + 1, paren_end - paren_start - 1);
            // 簡單處理：如果是以引號開始和結束，提取內容
            if(arg.size() >= 2 && arg.front() == '"' && arg.back() == '"'){
                std::string content = arg.substr(1, arg.size() - 2);
                auto v = enc_print(content);
                bc.insert(bc.end(), v.begin(), v.end());
            }
            // TODO: 處理變數連接如 "x = " + x
        }
        // 忽略其他行（變數聲明、函數等）
    }
    // 如果沒有行分隔符，嘗試處理整個字符串
    if(bc.empty()){
        std::string content = js;
        // 去除前後空白
        size_t start = content.find_first_not_of(" \t\r\n");
        if(start != std::string::npos){
            content = content.substr(start);
            size_t end = content.find_last_not_of(" \t\r\n");
            if(end != std::string::npos){
                content = content.substr(0, end + 1);
            }
        }
        
        auto pos = content.find("console.log");
        if(pos != std::string::npos){
            // 找到 console.log( 之後的內容
            size_t paren_start = content.find('(', pos);
            if(paren_start != std::string::npos){
                size_t paren_end = content.find(')', paren_start);
                if(paren_end != std::string::npos){
                    std::string arg = content.substr(paren_start + 1, paren_end - paren_start - 1);
                    // 簡單處理：如果是以引號開始和結束，提取內容
                    if(arg.size() >= 2 && arg.front() == '"' && arg.back() == '"'){
                        std::string text = arg.substr(1, arg.size() - 2);
                        auto v = enc_print(text);
                        bc.insert(bc.end(), v.begin(), v.end());
                    }
                }
            }
        }
    }
    return bc;
}

// ---- 翻譯器：中文自然語言 .zh → 位元碼（PoC：輸出字串 "…" / 其他原樣）----
static std::vector<uint8_t> translate_zh_to_bc(const std::string& zh){
    std::vector<uint8_t> bc; std::istringstream ss(zh); std::string line;
    auto ltrim=[](std::string& s){ size_t i=0; while(i<s.size() && (unsigned char)s[i]<=32) ++i; s.erase(0,i); };
    auto rtrim=[](std::string& s){ while(!s.empty() && (unsigned char)s.back()<=32) s.pop_back(); };
    while(std::getline(ss,line)){
        std::string raw=line; ltrim(line); rtrim(line);
        if(line.rfind("輸出字串(",0)==0){
            size_t start = line.find('(');
            size_t end = line.find_last_of(')');
            if(start != std::string::npos && end != std::string::npos && end > start){
                std::string content = line.substr(start + 1, end - start - 1);
                // 支援 「…」 或 "…"
                for(char& c: content){ if(c=='『'||c=='「') c='"'; if(c=='』'||c=='」') c='"'; }
                if(content.size()>=2 && content.front()=='"' && content.back()=='"'){
                    auto v=enc_print(content.substr(1,content.size()-2)); bc.insert(bc.end(), v.begin(), v.end()); continue;
                }
            }
        }
        auto v=enc_print(std::string("[未解析] ")+raw); bc.insert(bc.end(), v.begin(), v.end());
    }
    return bc;
}

// ---- 翻譯器：Python → 位元碼（PoC：print("…") / 其他忽略）----
static std::vector<uint8_t> translate_py_to_bc(const std::string& py){
    std::vector<uint8_t> bc; std::istringstream ss(py); std::string line;
    while(std::getline(ss,line)){
        // 去除前後空白
        size_t start = line.find_first_not_of(" \t");
        if(start == std::string::npos) continue;
        line = line.substr(start);
        if(line.empty()) continue;

        auto pos = line.find("print(");
        if(pos != std::string::npos){
            // 找到 print( 之後的內容
            size_t paren_start = line.find('(', pos);
            if(paren_start == std::string::npos) continue;
            size_t paren_end = line.find(')', paren_start);
            if(paren_end == std::string::npos) continue;

            std::string arg = line.substr(paren_start + 1, paren_end - paren_start - 1);
            // 簡單處理：如果是以引號開始和結束，提取內容
            if(arg.size() >= 2 && arg.front() == '"' && arg.back() == '"'){
                std::string content = arg.substr(1, arg.size() - 2);
                auto v = enc_print(content);
                bc.insert(bc.end(), v.begin(), v.end());
            }
        }
        // 忽略其他行（變數聲明、函數等）
    }
    return bc;
}

// ---- 翻譯器：Go → 位元碼（PoC：fmt.Println("…") / 其他忽略）----
static std::vector<uint8_t> translate_go_to_bc(const std::string& go){
    std::vector<uint8_t> bc; std::istringstream ss(go); std::string line;
    while(std::getline(ss,line)){
        // 去除前後空白
        size_t start = line.find_first_not_of(" \t");
        if(start == std::string::npos) continue;
        line = line.substr(start);
        if(line.empty()) continue;

        auto pos = line.find("fmt.Println(");
        if(pos != std::string::npos){
            // 找到 fmt.Println( 之後的內容
            size_t paren_start = line.find('(', pos);
            if(paren_start == std::string::npos) continue;
            size_t paren_end = line.find(')', paren_start);
            if(paren_end == std::string::npos) continue;

            std::string arg = line.substr(paren_start + 1, paren_end - paren_start - 1);
            // 簡單處理：如果是以引號開始和結束，提取內容
            if(arg.size() >= 2 && arg.front() == '"' && arg.back() == '"'){
                std::string content = arg.substr(1, arg.size() - 2);
                auto v = enc_print(content);
                bc.insert(bc.end(), v.begin(), v.end());
            }
        }
        // 忽略其他行（變數聲明、函數等）
    }
    return bc;
}

// ---- 翻譯器：Java → 位元碼（PoC：System.out.println("…") / 其他忽略）----
static std::vector<uint8_t> translate_java_to_bc(const std::string& java){
    std::vector<uint8_t> bc; std::istringstream ss(java); std::string line;
    while(std::getline(ss,line)){
        // 去除前後空白
        size_t start = line.find_first_not_of(" \t");
        if(start == std::string::npos) continue;
        line = line.substr(start);
        if(line.empty()) continue;

        auto pos = line.find("System.out.println(");
        if(pos != std::string::npos){
            // 找到 System.out.println( 之後的內容
            size_t paren_start = line.find('(', pos);
            if(paren_start == std::string::npos) continue;
            size_t paren_end = line.find(')', paren_start);
            if(paren_end == std::string::npos) continue;

            std::string arg = line.substr(paren_start + 1, paren_end - paren_start - 1);
            // 簡單處理：如果是以引號開始和結束，提取內容
            if(arg.size() >= 2 && arg.front() == '"' && arg.back() == '"'){
                std::string content = arg.substr(1, arg.size() - 2);
                auto v = enc_print(content);
                bc.insert(bc.end(), v.begin(), v.end());
            }
        }
        // 忽略其他行（變數聲明、函數等）
    }
    return bc;
}

// ---- pack：寫入 version 與 CRC、印自證訊息 ----
static int pack_payload_to_exe(const std::filesystem::path& output_exe,
                               const std::vector<uint8_t>& bc,
#ifdef _WIN32
                               const std::filesystem::path& self_exe = []{
                                   wchar_t pathW[MAX_PATH]{0};
                                   GetModuleFileNameW(nullptr, pathW, MAX_PATH);
                                   return std::filesystem::path(pathW);
                               }()
#else
                               const std::filesystem::path& self_exe = "/proc/self/exe"
#endif
){
    if(!file_copy(self_exe, output_exe)) {
        std::fprintf(stderr, "[selfhost] copy self -> out failed\n"); return 4;
    }
    std::ofstream out(output_exe, std::ios::binary | std::ios::app);
    if(!out){ std::fprintf(stderr, "[selfhost] open out for append failed\n"); return 5; }
    uint64_t off = (uint64_t)std::filesystem::file_size(output_exe);
    out.write((const char*)bc.data(), (std::streamsize)bc.size());

    Trailer tr{};
    tr.magic = SH_MAGIC;
    tr.payload_size = (uint64_t)bc.size();
    tr.payload_offset = off;
    tr.version = SH_VERSION;                 // <== New
    tr.crc32 = crc32(bc.data(), bc.size());  // <== New

    out.write((const char*)&tr, sizeof(tr)); out.flush();

    std::printf("[selfhost] packed -> %s (v%u, size=%llu, crc=%08X)\n",
        output_exe.string().c_str(),
        tr.version,
        (unsigned long long)tr.payload_size,
        tr.crc32);

    return 0;
}

// ---- 對外入口：由 CLI 呼叫 ----
static int pack_from_file(const std::string& lang, const std::filesystem::path& in, const std::filesystem::path& out){
    std::string src = read_all(in);
    std::vector<uint8_t> bc;
    if(lang=="js" || lang=="javascript") bc = translate_js_to_bc(src);
    else if(lang=="py" || lang=="python") bc = translate_py_to_bc(src);
    else if(lang=="go")                   bc = translate_go_to_bc(src);
    else if(lang=="java")                 bc = translate_java_to_bc(src);
    else if(lang=="zh")                   bc = translate_zh_to_bc(src);
    else { std::fprintf(stderr, "[selfhost] unsupported lang: %s\n", lang.c_str()); return 2; }
    return pack_payload_to_exe(out, bc);
}

// ---- 對外入口不變（pack_from_file），下面加 verify 功能即可 ----
static int verify_exe(const std::filesystem::path& exe){
    auto R = read_payload_from_file(exe);
    if(!R.ok){
        std::fprintf(stderr,"[selfhost] no payload in: %s\n", exe.string().c_str());
        return 2;
    }
    std::printf("[selfhost] verify: %s\n", exe.string().c_str());
    std::printf("  version : %u\n", R.tr.version);
    std::printf("  size    : %llu bytes\n", (unsigned long long)R.tr.payload_size);
    std::printf("  offset  : %llu\n", (unsigned long long)R.tr.payload_offset);
    std::printf("  crc32   : %08X (%s)\n", R.tr.crc32, R.crc_ok?"OK":"BAD");
    return R.crc_ok? 0 : 3;
}

// ---- handle selfhost explain command ----
static int handle_selfhost_explain(int argc, char** argv) {
    if (argc < 4) {
        std::puts("Usage:\n  zhcl_universal selfhost explain <input.(js|py|go|java|zh)>");
        return 2;
    }
    std::filesystem::path in = argv[3];
    std::string src = read_all(in); // 同命名空間內可直接用
    std::vector<uint8_t> bc;
    auto ext = in.extension().string();
    if (ext == ".js")      bc = translate_js_to_bc(src);
    else if (ext == ".py") bc = translate_py_to_bc(src);
    else if (ext == ".go") bc = translate_go_to_bc(src);
    else if (ext == ".java") bc = translate_java_to_bc(src);
    else if (ext == ".zh") bc = translate_zh_to_bc(src);
    else {
        std::fprintf(stderr, "[selfhost] unsupported input: %s\n", ext.c_str());
        return 2;
    }
    disassemble_bc(bc);
    return 0;
}

} // namespace selfhost

// Use explicit std:: prefix instead of using namespace std
namespace fs = std::filesystem;

// Forward declarations
class CompilerRegistry;
class LanguageProcessor;
class CppProcessor;
class JavaProcessor;
class PythonProcessor;
class GoProcessor;
class RustProcessor;

// JVM Bytecode Writer (Big-Endian)
struct W {
    std::vector<uint8_t> buf;
    void u1(uint8_t x) { buf.push_back(x); }
    void u2(uint16_t x) { buf.push_back((x >> 8) & 0xFF); buf.push_back(x & 0xFF); }
    void u4(uint32_t x) {
        buf.push_back((x >> 24) & 0xFF); buf.push_back((x >> 16) & 0xFF);
        buf.push_back((x >> 8) & 0xFF); buf.push_back(x & 0xFF);
    }
    void bytes(const void* p, size_t n) {
        auto c = (const uint8_t*)p; buf.insert(buf.end(), c, c + n);
    }
    void utf8(const std::string& s) { u1(1); u2((uint16_t)s.size()); bytes(s.data(), s.size()); }
};

// ============================================================================
// LANGUAGE PROCESSORS - Handle compilation for each language
// ============================================================================

class LanguageProcessor {
public:
    virtual ~LanguageProcessor() = default;
    virtual bool can_handle(const std::string& ext) = 0;
    virtual int compile(const std::string& input, const std::string& output,
                       const std::vector<std::string>& flags, bool verbose) = 0;
    virtual int run(const std::string& executable, bool verbose) = 0;
    virtual std::string get_default_output(const std::string& input) = 0;
};

// ============================================================================
// COMPILER REGISTRY SYSTEM - Detects and manages all compilers
// ============================================================================

class CompilerRegistry {
public:
    struct Compiler {
        std::string name;
        std::string command;
        std::vector<std::string> flags;
        std::set<std::string> supported_languages;
        bool available = false;
        int priority = 0; // Higher = preferred
    };

    std::map<std::string, Compiler> compilers;

    void detect_compilers() {
        // C/C++ Compilers
        detect_compiler("msvc", "cl", {"C", "C++"}, {"-nologo", "-utf-8", "-EHsc", "-std:c++17"}, 100);
        detect_compiler("gcc", "gcc", {"C", "C++"}, {"-Wall", "-std=c++17"}, 90);
        detect_compiler("g++", "g++", {"C++"}, {"-Wall", "-std=c++17"}, 90);
        detect_compiler("clang", "clang", {"C", "C++"}, {"-Wall", "-std=c++17"}, 95);
        detect_compiler("clang++", "clang++", {"C++"}, {"-Wall", "-std=c++17"}, 95);

        // JVM Languages
        detect_compiler("javac", "javac", {"Java"}, {"-g"}, 80);
        detect_compiler("kotlin", "kotlinc", {"Kotlin"}, {}, 70);
        detect_compiler("scala", "scalac", {"Scala"}, {}, 70);

        // Systems Languages
        detect_compiler("rustc", "rustc", {"Rust"}, {"-O"}, 85);
        detect_compiler("go", "go", {"Go"}, {}, 85);
        detect_compiler("swift", "swiftc", {"Swift"}, {}, 75);

        // Scripting Languages
        detect_compiler("python", "python", {"Python"}, {}, 60);
        detect_compiler("node", "node", {"JavaScript"}, {}, 60);
        detect_compiler("tsc", "tsc", {"TypeScript"}, {}, 65);
        detect_compiler("ruby", "ruby", {"Ruby"}, {}, 60);
        detect_compiler("perl", "perl", {"Perl"}, {}, 60);

        // .NET
        detect_compiler("dotnet", "dotnet", {"C#", "F#", "VB.NET"}, {}, 80);
        detect_compiler("mono", "mcs", {"C#"}, {}, 70);

        // Special cases
        detect_compiler("zhcc", "zhcc_cpp.exe", {"Chinese"}, {"--cc"}, 50);
    }

private:
    void detect_compiler(const std::string& name, const std::string& cmd, const std::set<std::string>& langs,
                        const std::vector<std::string>& flags, int priority) {
        std::string test_cmd;
        if (name == "msvc") {
            test_cmd = cmd + " /? >nul 2>&1";
        } else {
            test_cmd = cmd + " --version >nul 2>&1";
        }
        bool available = (system(test_cmd.c_str()) == 0);

        compilers[name] = {name, cmd, flags, langs, available, priority};

        if (!available) {
            // Try alternative test commands
            std::vector<std::string> alt_tests;
            if (name == "msvc") {
                alt_tests = {" /help", ""};
            } else {
                alt_tests = {" --help", " -v", " -version", ""};
            }
            for (const auto& alt : alt_tests) {
                if (system((cmd + alt + " >nul 2>&1").c_str()) == 0) {
                    compilers[name].available = true;
                    break;
                }
            }
        }
    }
};

class CppProcessor : public LanguageProcessor {
public:
    CompilerRegistry* registry;

    bool can_handle(const std::string& ext) override {
        return ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx";
    }

    int compile(const std::string& input, const std::string& output,
               const std::vector<std::string>& flags, bool verbose) override {
        // Find best available C++ compiler
        std::string compiler_cmd;
        std::vector<std::string> default_flags;

        if (registry->compilers["msvc"].available) {
            compiler_cmd = "cl";
            default_flags = {"/nologo", "/utf-8", "/EHsc", "/std:c++17"};
        } else if (registry->compilers["clang++"].available) {
            compiler_cmd = "clang++";
            default_flags = {"-Wall", "-std=c++17"};
        } else if (registry->compilers["g++"].available) {
            compiler_cmd = "g++";
            default_flags = {"-Wall", "-std=c++17"};
        } else {
            // Try hardcoded MSVC path as fallback
            std::string msvc_path = "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Tools\\MSVC\\14.29.30133\\bin\\Hostx64\\x64\\cl.exe\"";
            std::string test_cmd = msvc_path + " /? >nul 2>&1";
            if (system(test_cmd.c_str()) == 0) {
                compiler_cmd = msvc_path;
                default_flags = {"/nologo", "/utf-8", "/EHsc", "/std:c++17"};
            } else {
                compiler_cmd = "cc";
                default_flags = {"-std=c++17"};
            }
        }

        std::string cmd = compiler_cmd;
        for (const auto& flag : default_flags) cmd += " " + flag;
        for (const auto& flag : flags) cmd += " " + flag;
        cmd += " \"" + input + "\"";

        if (!output.empty()) {
            if (compiler_cmd.find("cl") != std::string::npos) {
                cmd += " /Fe:\"" + output + "\"";
            } else {
                cmd += " -o \"" + output + "\"";
            }
        }

        if (verbose) std::cout << "Compiling C/C++: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    int run(const std::string& executable, bool verbose) override {
        std::string cmd = "\"" + executable + "\"";
        if (verbose) std::cout << "Running: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string& input) override {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }
};

class JavaProcessor : public LanguageProcessor {
public:
    CompilerRegistry* registry;

    bool can_handle(const std::string& ext) override {
        return ext == ".java";
    }

    int compile(const std::string& input, const std::string& output,
               const std::vector<std::string>& flags, bool verbose) override {
        // Try javac first
        if (registry->compilers["javac"].available) {
            std::string cmd = "javac";
            for (const auto& flag : flags) cmd += " " + flag;
            cmd += " \"" + input + "\"";
            if (verbose) std::cout << "Compiling Java with javac: " << cmd << std::endl;
            return system(cmd.c_str());
        } else {
            // Fallback to embedded bytecode generator
            return generate_bytecode(input, output, verbose);
        }
    }

    int run(const std::string& executable, bool verbose) override {
        std::string class_name = executable.substr(0, executable.find_last_of('.'));
        std::string cmd = "java " + class_name;
        if (verbose) std::cout << "Running Java: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string& input) override {
        return input.substr(0, input.find_last_of('.')) + ".class";
    }

private:
    int generate_bytecode(const std::string& input, const std::string& output, bool verbose) {
        // Simple Java bytecode generator - only supports basic System.out.println("...");

        std::ifstream in(input);
        if (!in) {
            std::cerr << "Cannot open Java file: " << input << std::endl;
            return 1;
        }

        std::string line;
        std::string print_content;
        bool found_println = false;

        while (std::getline(in, line)) {
            // Look for System.out.println("...");
            size_t pos = line.find("System.out.println(");
            if (pos != std::string::npos) {
                size_t start = line.find('"', pos);
                size_t end = line.find('"', start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    print_content = line.substr(start + 1, end - start - 1);
                    found_println = true;
                    break;
                }
            }
        }

        in.close();

        if (!found_println) {
            // Fallback to Hello World
            print_content = "Hello, World!";
        }

        // Generate bytecode for System.out.println(print_content);
        std::vector<uint8_t> classfile;
        generate_println_class(classfile, print_content);

        std::string classfile_name = output.empty() ?
            input.substr(0, input.find_last_of('.')) + ".class" : output;

        std::ofstream out(classfile_name, std::ios::binary);
        out.write((char*)classfile.data(), classfile.size());
        out.close();

        if (verbose) std::cout << "Generated Java bytecode: " << classfile_name << std::endl;
        return 0;
    }

    // Embedded JVM bytecode generator (simplified)
    void generate_println_class(std::vector<uint8_t>& out, const std::string& message) {
        // This is a simplified version - in reality you'd parse the Java file
        // For now, just generate a Hello World class
        W w;
        // ... (same bytecode generation code as before)
        w.u4(0xCAFEBABE); w.u2(0); w.u2(49);
        w.u2(28);
        // ... (rest of the bytecode)
        w.utf8("HelloWorld");
        w.u1(7); w.u2(1);
        w.utf8("java/lang/Object");
        w.u1(7); w.u2(3);
        w.utf8("java/lang/System");
        w.u1(7); w.u2(5);
        w.utf8("out");
        w.utf8("Ljava/io/PrintStream;");
        w.u1(12); w.u2(7); w.u2(8);
        w.u1(9); w.u2(6); w.u2(9);
        w.utf8("java/io/PrintStream");
        w.u1(7); w.u2(11);
        w.utf8("println");
        w.utf8("(Ljava/lang/string;)V");
        w.u1(12); w.u2(13); w.u2(14);
        w.u1(10); w.u2(12); w.u2(15);
        w.utf8(message); // The message
        w.u1(8); w.u2(17);
        w.utf8("<init>");
        w.utf8("()V");
        w.u1(12); w.u2(19); w.u2(20);
        w.u1(10); w.u2(4); w.u2(21);
        w.utf8("Code");
        w.utf8("main");
        w.utf8("([Ljava/lang/string;)V");
        w.utf8("SourceFile");
        w.utf8("HelloWorld.java");
        w.u2(0x0021);
        w.u2(2);
        w.u2(4);
        w.u2(0);
        w.u2(0);
        w.u2(2);
        w.u2(0x0001);
        w.u2(19);
        w.u2(20);
        w.u2(1);
        w.u2(23);
        w.u4(17);
        w.u2(1);
        w.u2(1);
        w.u4(5);
        w.u1(0x2a);
        w.u1(0xb7); w.u2(22);
        w.u1(0xb1);
        w.u2(0);
        w.u2(0);
        w.u2(0x0009);
        w.u2(24);
        w.u2(25);
        w.u2(1);
        w.u2(23);
        w.u4(21);
        w.u2(2);
        w.u2(1);
        w.u4(9);
        w.u1(0xb2); w.u2(10);
        w.u1(0x12); w.u1(18);
        w.u1(0xb6); w.u2(16);
        w.u1(0xb1);
        w.u2(0);
        w.u2(0);
        w.u2(1);
        w.u2(26);
        w.u4(2);
        w.u2(27);
        out.swap(w.buf);
    }
};

class ChineseProcessor : public LanguageProcessor {
public:
    CompilerRegistry* registry;

    bool can_handle(const std::string& ext) override {
        return ext == ".zh";
    }

    int compile(const std::string& input, const std::string& output,
               const std::vector<std::string>& flags, bool verbose) override {
        // Translate .zh to .cpp using built-in translator
        std::string cpp_file = output.empty() ?
            input.substr(0, input.find_last_of('.')) + ".cpp" : output.substr(0, output.find_last_of('.')) + ".cpp";

        if (translate_zh_to_cpp(input, cpp_file, verbose) != 0) {
            return 1;
        }

        // Then compile the generated C++ file
        CppProcessor cpp_processor;
        cpp_processor.registry = registry;
        std::string exe_output = output.empty() ?
            input.substr(0, input.find_last_of('.')) + ".exe" : output;

        return cpp_processor.compile(cpp_file, exe_output, flags, verbose);
    }

    int run(const std::string& executable, bool verbose) override {
        std::string cmd = "\"" + executable + "\"";
        if (verbose) std::cout << "Running Chinese program: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string& input) override {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }

private:
    int translate_zh_to_cpp(const std::string& zh_file, const std::string& cpp_file, bool verbose) {
        std::ifstream in(zh_file);
        std::ofstream out(cpp_file);
        std::string line;

        out << "#include \"../include/chinese.h\"\n";
        out << "#include <iostream>\n";
        out << "#include <stdlib.h>\n";
        out << "#include <math.h>\n";
        out << "#include <time.h>\n\n";

        std::vector<std::string> global_functions;
        std::string main_code;
        bool in_function = false;
        std::string current_function;

        while (std::getline(in, line)) {
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line.substr(0, 2) == "//") {
                if (!line.empty()) {
                    if (in_function) current_function += line + "\n";
                    else main_code += line + "\n";
                }
                continue;
            }

            // Function definition
            if (line.substr(0, 3) == "函數 " && line.find('(') != std::string::npos && !(line.size() >= 1 && line[line.size() - 1] == ';')) {
                in_function = true;
                std::string func_def = line.substr(3);
                // Replace parameter types
                func_def = replace_all(func_def, "字串", "char*");
                func_def = replace_all(func_def, "整數", "int");
                func_def = replace_all(func_def, "小數", "double");
                // Replace parameter names
                func_def = replace_all(func_def, "半徑", "radius");
                func_def = replace_all(func_def, "面積", "area");
                func_def = replace_all(func_def, "結果", "result");
                func_def = replace_all(func_def, "年齡", "age");
                func_def = replace_all(func_def, "名字", "name");
                func_def = replace_all(func_def, "圓周率值", "pi_value");
                func_def = replace_all(func_def, "訊息", "message");
                func_def = replace_all(func_def, "問候", "greet");
                current_function = "void " + func_def + " {\n";
                continue;
            }

            // Function end
            if (line == "函數結束") {
                if (in_function) {
                    current_function += "}\n";
                    global_functions.push_back(current_function);
                    current_function = "";
                    in_function = false;
                }
                continue;
            }

            // Process function body or main code
            if (in_function) {
                current_function += process_line(line) + "\n";
            } else {
                main_code += process_line(line) + "\n";
            }
        }

        // Write global functions
        for (const auto& func : global_functions) {
            out << func << "\n";
        }

        // Write main function
        out << "int main() {\n";
        out << "    初始化中文環境();\n";
        out << main_code;
        out << "    return 0;\n";
        out << "}\n";

        in.close();
        out.close();

        if (verbose) std::cout << "Translated " << zh_file << " to " << cpp_file << std::endl;
        return 0;
    }

    std::string process_line(const std::string& line) {
        std::string result = line;

        // Variable declarations
        if (result.substr(0, 3) == "整數 ") {
            result = "int " + result.substr(3);
        } else if (result.substr(0, 3) == "小數 ") {
            result = "double " + result.substr(3);
        } else if (result.substr(0, 3) == "字串 ") {
            result = "char* " + result.substr(3);
        }

        // Control structures
        result = replace_all(result, "如果", "if");
        result = replace_all(result, "則", "");
        result = replace_all(result, "否則", "else");
        result = replace_all(result, "當", "while");
        result = replace_all(result, "對於", "for");
        result = replace_all(result, "印出", "printf");
        result = replace_all(result, "輸出字串", "printf");
        result = replace_all(result, "輸出整數", "printf");
        result = replace_all(result, "輸出小數", "printf");
        result = replace_all(result, "返回", "return");
        result = replace_all(result, "結束", "}");

        // Operators
        result = replace_all(result, "等於", "==");
        result = replace_all(result, "大於", ">");
        result = replace_all(result, "小於", "<");
        result = replace_all(result, "大於等於", ">=");
        result = replace_all(result, "小於等於", "<=");
        result = replace_all(result, "不等於", "!=");
        result = replace_all(result, "且", "&&");
        result = replace_all(result, "或", "||");
        result = replace_all(result, "非", "!");

        // Arithmetic
        result = replace_all(result, "加", "+");
        result = replace_all(result, "減", "-");
        result = replace_all(result, "乘", "*");
        result = replace_all(result, "除", "/");
        result = replace_all(result, "取餘", "%");

        // Assignment
        result = replace_all(result, "等於", "=");

        // Constants and variables
        result = replace_all(result, "圓周率", "3.141592653589793");
        result = replace_all(result, "半徑", "radius");
        result = replace_all(result, "面積", "area");
        result = replace_all(result, "結果", "result");
        result = replace_all(result, "年齡", "age");
        result = replace_all(result, "名字", "name");
        result = replace_all(result, "圓周率值", "pi_value");
        result = replace_all(result, "訊息", "message");
        result = replace_all(result, "問候", "greet");
        result = replace_all(result, "真", "true");
        result = replace_all(result, "假", "false");
        result = replace_all(result, "空", "NULL");

        // Functions
        result = replace_all(result, "隨機數", "rand()");
        result = replace_all(result, "平方根", "sqrt");
        result = replace_all(result, "絕對值", "abs");
        result = replace_all(result, "正弦", "sin");
        result = replace_all(result, "餘弦", "cos");
        result = replace_all(result, "正切", "tan");

        // Add semicolon if not present and not a control structure
        if (!result.empty() && result.back() != ';' && result.back() != '{' && result.back() != '}') {
            result += ';';
        }

        return result;
    }

    std::string replace_all(std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }

    // Helper function to trim whitespace from both ends of a string
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }
};

class PythonProcessor : public LanguageProcessor {
public:
    CompilerRegistry* registry;

    bool can_handle(const std::string& ext) override { return ext == ".py"; }

    int compile(const std::string& input, const std::string& output,
               const std::vector<std::string>& flags, bool verbose) override {
        // Translate .py to .cpp using built-in translator
        std::string cpp_file = output.empty() ?
            input.substr(0, input.find_last_of('.')) + ".cpp" : output.substr(0, output.find_last_of('.')) + ".cpp";

        if (translate_py_to_cpp(input, cpp_file, verbose) != 0) {
            return 1;
        }

        // Then compile the generated C++ file
        CppProcessor cpp_processor;
        cpp_processor.registry = registry;
        std::string exe_output = output.empty() ?
            input.substr(0, input.find_last_of('.')) + ".exe" : output;

        return cpp_processor.compile(cpp_file, exe_output, flags, verbose);
    }

    int run(const std::string& executable, bool verbose) override {
        std::string cmd = "\"" + executable + "\"";
        if (verbose) std::cout << "Running Python program: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string& input) override {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }

private:
    int translate_py_to_cpp(const std::string& py_file, const std::string& cpp_file, bool verbose) {
        std::ifstream in(py_file);
        std::ofstream out(cpp_file);
        std::string line;

        out << "#include <iostream>\n";
        out << "#include <string>\n";
        out << "#include <vector>\n\n";

        std::string main_code;

        while (std::getline(in, line)) {
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line.substr(0, 1) == "#") {
                continue;
            }

            // Basic Python to C++ translation (very simplified)
            if (line.find("print(") != std::string::npos) {
                // print("hello") -> std::cout << "hello" << std::endl;
                size_t start = line.find('"');
                size_t end = line.find('"', start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    std::string content = line.substr(start, end - start + 1);
                    main_code += "    std::cout << " + content + " << std::endl;\n";
                }
            } else if (line.find("=") != std::string::npos && line.find("int(") == std::string::npos) {
                // x = 5 -> int x = 5;
                size_t eq_pos = line.find('=');
                std::string var = trim(line.substr(0, eq_pos));
                std::string value = trim(line.substr(eq_pos + 1));
                main_code += "    int " + var + " = " + value + ";\n";
            } else {
                // Other lines, pass through with comment
                main_code += "    // " + line + "\n";
            }
        }

        out << "int main() {\n";
        out << main_code;
        out << "    return 0;\n";
        out << "}\n";

        in.close();
        out.close();

        if (verbose) std::cout << "Translated " + py_file + " to " + cpp_file << std::endl;
        return 0;
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }
};

class GoProcessor : public LanguageProcessor {
public:
    CompilerRegistry* registry;

    bool can_handle(const std::string& ext) override { return ext == ".go"; }

    int compile(const std::string& input, const std::string& output,
               const std::vector<std::string>& flags, bool verbose) override {
        // Translate .go to .cpp using built-in translator
        std::string cpp_file = output.empty() ?
            input.substr(0, input.find_last_of('.')) + ".cpp" : output.substr(0, output.find_last_of('.')) + ".cpp";

        if (translate_go_to_cpp(input, cpp_file, verbose) != 0) {
            return 1;
        }

        // Then compile the generated C++ file
        CppProcessor cpp_processor;
        cpp_processor.registry = registry;
        std::string exe_output = output.empty() ?
            input.substr(0, input.find_last_of('.')) + ".exe" : output;

        return cpp_processor.compile(cpp_file, exe_output, flags, verbose);
    }

    int run(const std::string& executable, bool verbose) override {
        std::string cmd = "\"" + executable + "\"";
        if (verbose) std::cout << "Running Go program: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string& input) override {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }

private:
    int translate_go_to_cpp(const std::string& go_file, const std::string& cpp_file, bool verbose) {
        std::ifstream in(go_file);
        std::ofstream out(cpp_file);
        std::string line;

        out << "#include <iostream>\n";
        out << "#include <string>\n";
        out << "#include <vector>\n";
        out << "#include <thread>\n";
        out << "#include <mutex>\n\n";

        std::string main_code;
        std::vector<std::string> functions;

        while (std::getline(in, line)) {
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line.substr(0, 2) == "//") {
                continue;
            }

            // Package declaration
            if (line.substr(0, 8) == "package ") {
                // Skip for now
                continue;
            }

            // Import statements
            if (line.substr(0, 7) == "import ") {
                // Skip for now
                continue;
            }

            // Function declarations
            if (line.substr(0, 5) == "func ") {
                std::string func_line = translate_function_declaration(line);
                functions.push_back(func_line);
                // Read function body
                std::string func_body;
                int brace_count = 0;
                bool in_func = true;
                while (in_func && std::getline(in, line)) {
                    line = trim(line);
                    if (line.find('{') != std::string::npos) brace_count++;
                    if (line.find('}') != std::string::npos) brace_count--;
                    func_body += translate_go_statement(line) + "\n";
                    if (brace_count == 0) in_func = false;
                }
                functions.back() += func_body + "}\n";
                continue;
            }

            // Main code
            main_code += translate_go_statement(line) + "\n";
        }

        // Write functions
        for (const auto& func : functions) {
            out << func << "\n";
        }

        // Write main function
        out << "int main() {\n";
        out << main_code;
        out << "    return 0;\n";
        out << "}\n";

        in.close();
        out.close();

        if (verbose) std::cout << "Translated " << go_file << " to " << cpp_file << std::endl;
        return 0;
    }

    std::string translate_function_declaration(const std::string& line) {
        // func main() -> void main()
        // func add(a int, b int) int -> int add(int a, int b)
        std::string result = line;
        result = replace_all(result, "func ", "");
        // Simple replacement - expand for full Go syntax
        result = replace_all(result, "int", "int");
        result = replace_all(result, "string", "std::string");
        result = replace_all(result, "bool", "bool");
        // Assume return type at end
        size_t last_space = result.find_last_of(' ');
        if (last_space != std::string::npos) {
            std::string return_type = result.substr(last_space + 1);
            std::string func_sig = result.substr(0, last_space);
            result = return_type + " " + func_sig;
        }
        return result + " {";
    }

    std::string translate_go_statement(const std::string& line) {
        std::string result = line;

        // Variable declarations: var x int = 5 -> int x = 5;
        if (result.substr(0, 4) == "var ") {
            result = result.substr(4);
            // Assume format: name type = value
            // Simple: replace with C++ style
        }

        // Print statements: fmt.Println("hello") -> std::cout << "hello" << std::endl;
        if (result.find("fmt.Println") != std::string::npos) {
            size_t start = result.find('"');
            size_t end = result.find('"', start + 1);
            if (start != std::string::npos && end != std::string::npos) {
                std::string content = result.substr(start, end - start + 1);
                result = "    std::cout << " + content + " << std::endl;";
            }
        }

        // Basic replacements
        result = replace_all(result, ":=", "=");  // Short variable declaration

        return result;
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }

    std::string replace_all(std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }
};

class RustProcessor : public LanguageProcessor {
public:
    bool can_handle(const std::string& ext) override { return ext == ".rs"; }
    int compile(const std::string& input, const std::string& output, const std::vector<std::string>& flags, bool verbose) override {
        std::string cmd = "rustc";
        for (const auto& flag : flags) cmd += " " + flag;
        cmd += " \"" + input + "\"";
        if (!output.empty()) cmd += " -o \"" + output + "\"";
        if (verbose) std::cout << "Compiling Rust: " << cmd << std::endl;
        return system(cmd.c_str());
    }
    int run(const std::string& executable, bool verbose) override {
        std::string cmd = "\"" + executable + "\"";
        if (verbose) std::cout << "Running Rust: " << cmd << std::endl;
        return system(cmd.c_str());
    }
    std::string get_default_output(const std::string& input) override {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }
};

class JSProcessor : public LanguageProcessor {
public:
    CompilerRegistry* registry;

    bool can_handle(const std::string& ext) override { return ext == ".js"; }

    int compile(const std::string& input, const std::string& output,
               const std::vector<std::string>& flags, bool verbose) override {
        // Translate .js to .cpp using built-in translator
        std::string cpp_file = output.empty() ?
            input.substr(0, input.find_last_of('.')) + ".cpp" : output.substr(0, output.find_last_of('.')) + ".cpp";

        if (translate_js_to_cpp(input, cpp_file, verbose) != 0) {
            return 1;
        }

        // Then compile the generated C++ file
        CppProcessor cpp_processor;
        cpp_processor.registry = registry;
        return cpp_processor.compile(cpp_file, output, flags, verbose);
    }

    int run(const std::string& executable, bool verbose) override {
        std::string cmd = "node \"" + executable + "\"";
        if (verbose) std::cout << "Running JavaScript: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string& input) override { return input; }

private:
    int translate_js_to_cpp(const std::string& js_file, const std::string& cpp_file, bool verbose) {
        std::ifstream in(js_file);
        std::ofstream out(cpp_file);
        std::string line;

        out << "#include <iostream>\n";
        out << "#include <string>\n";
        out << "#include <vector>\n\n";

        std::string main_code;

        while (std::getline(in, line)) {
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line.substr(0, 2) == "//") {
                continue;
            }

            // Basic JavaScript to C++ translation (very simplified)
            if (line.find("console.log(") != std::string::npos) {
                // console.log("hello") -> std::cout << "hello" << std::endl;
                size_t start = line.find('"');
                size_t end = line.find('"', start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    std::string content = line.substr(start, end - start + 1);
                    main_code += "    std::cout << " + content + " << std::endl;\n";
                }
            } else if (line.find("let ") != std::string::npos || line.find("const ") != std::string::npos || line.find("var ") != std::string::npos) {
                // let x = 5 -> int x = 5;
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos) {
                    std::string var_part = trim(line.substr(0, eq_pos));
                    std::string value = trim(line.substr(eq_pos + 1));
                    // Remove semicolon if present
                    if (!value.empty() && value.back() == ';') value.pop_back();
                    
                    // Extract variable name (after let/const/var and space)
                    size_t space_pos = var_part.find(' ');
                    if (space_pos != std::string::npos) {
                        std::string var_name = trim(var_part.substr(space_pos + 1));
                        main_code += "    int " + var_name + " = " + value + ";\n";
                    }
                }
            } else {
                // Other lines, pass through with comment
                main_code += "    // " + line + "\n";
            }
        }

        out << "int main() {\n";
        out << main_code;
        out << "    return 0;\n";
        out << "}\n";

        in.close();
        out.close();

        if (verbose) std::cout << "Translated " + js_file + " to " + cpp_file << std::endl;
        return 0;
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }
};

// ============================================================================
// COMPILER COMPATIBILITY LAYER - Maintains traditional compiler habits
// ============================================================================

class CompilerCompatibilityLayer {
public:
    CompilerRegistry* registry;

    // Parse traditional compiler arguments and translate to zhcl format
    int handle_traditional_args(int argc, char** argv) {
        if (argc < 2) return -1; // Not a traditional compiler call

        std::string compiler_name = argv[0];
        std::string basename = fs::path(compiler_name).filename().string();

        // map common compiler names to our system
        if (basename == "cl" || basename == "cl.exe") {
            return handle_msvc_args(argc, argv);
        } else if (basename == "gcc" || basename == "g++" || basename == "cc" || basename == "c++") {
            return handle_gcc_args(argc, argv);
        } else if (basename == "javac" || basename == "javac.exe") {
            return handle_javac_args(argc, argv);
        } else if (basename == "rustc" || basename == "rustc.exe") {
            return handle_rustc_args(argc, argv);
        } else if (basename == "go" || basename == "go.exe") {
            return handle_go_args(argc, argv);
        } else if (basename == "python" || basename == "python.exe" || basename == "python3") {
            return handle_python_args(argc, argv);
        }

        return -1; // Not recognized
    }

private:
    // MSVC-style arguments: cl [options] file.cpp
    int handle_msvc_args(int argc, char** argv) {
        std::vector<std::string> files;
        std::string output;
        std::vector<std::string> includes;
        std::vector<std::string> defines;
        std::vector<std::string> flags;
        bool compile_only = false;
        bool verbose = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "/c") {
                compile_only = true;
            } else if (arg == "/Fe" && i + 1 < argc) {
                output = argv[++i];
            } else if (arg.substr(0, 2) == "/I") {
                includes.push_back(arg.substr(2));
            } else if (arg.substr(0, 2) == "/D") {
                defines.push_back(arg.substr(2));
            } else if (arg == "/nologo") {
                // Silent mode
            } else if (arg == "/utf-8") {
                flags.push_back("-utf-8");
            } else if (arg == "/EHsc") {
                flags.push_back("-EHsc");
            } else if (arg == "/std:c++17") {
                flags.push_back("-std=c++17");
            } else if (arg.substr(0, 1) != "/") {
                // Assume it's a file
                files.push_back(arg);
            }
        }

        if (files.empty()) {
            std::cerr << "cl: no input files" << std::endl;
            return 1;
        }

        // Compile each file
        for (const auto& file : files) {
            std::string ext = fs::path(file).extension().string();
            if (ext != ".cpp" && ext != ".c" && ext != ".cc") {
                std::cerr << "cl: unsupported file type: " << file << std::endl;
                continue;
            }

            std::string actual_output = output;
            if (actual_output.empty()) {
                if (compile_only) {
                    actual_output = file.substr(0, file.find_last_of('.')) + ".obj";
                } else {
                    actual_output = file.substr(0, file.find_last_of('.')) + ".exe";
                }
            }

            // Add include paths
            for (const auto& inc : includes) {
                flags.push_back("-I" + inc);
            }

            // Add defines
            for (const auto& def : defines) {
                flags.push_back("-D" + def);
            }

            CppProcessor processor;
            processor.registry = registry;
            int ret = processor.compile(file, actual_output, flags, verbose);

            if (ret != 0) {
                return ret;
            }
        }

        return 0;
    }

    // GCC-style arguments: gcc [options] file.c -o output
    int handle_gcc_args(int argc, char** argv) {
        vector<string> files;
        string output;
        vector<string> includes;
        vector<string> defines;
        vector<string> flags;
        bool compile_only = false;
        bool verbose = false;

        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];

            if (arg == "-c") {
                compile_only = true;
            } else if (arg == "-o" && i + 1 < argc) {
                output = argv[++i];
            } else if (arg.substr(0, 2) == "-I") {
                includes.push_back(arg.substr(2));
            } else if (arg.substr(0, 2) == "-D") {
                defines.push_back(arg.substr(2));
            } else if (arg == "-Wall" || arg == "-Wextra" || arg == "-g" || arg == "-O2") {
                flags.push_back(arg);
            } else if (arg.substr(0, 5) == "-std=") {
                flags.push_back(arg);
            } else if (arg.substr(0, 1) != "-") {
                // Assume it's a file
                files.push_back(arg);
            }
        }

        if (files.empty()) {
            cerr << "gcc: no input files" << endl;
            return 1;
        }

        // Compile each file
        for (size_t idx = 0; idx < files.size(); ++idx) {
            const auto& file = files[idx];
            string ext = fs::path(file).extension().string();

            string actual_output;
            if (output.empty()) {
                if (compile_only) {
                    actual_output = file.substr(0, file.find_last_of('.')) + ".o";
                } else if (files.size() == 1) {
                    actual_output = "a.out";
                } else {
                    actual_output = file.substr(0, file.find_last_of('.')) + ".o";
                }
            } else {
                if (files.size() > 1) {
                    // Multiple files with single output = link them
                    if (idx == files.size() - 1) {
                        // Last file, do the linking
                        actual_output = output;
                    } else {
                        continue; // Skip intermediate files
                    }
                } else {
                    actual_output = output;
                }
            }

            // Add include paths
            for (const auto& inc : includes) {
                flags.push_back("-I" + inc);
            }

            // Add defines
            for (const auto& def : defines) {
                flags.push_back("-D" + def);
            }

            CppProcessor processor;
            processor.registry = registry;
            int ret = processor.compile(file, actual_output, flags, verbose);

            if (ret != 0) {
                return ret;
            }
        }

        return 0;
    }

    // JavaC-style arguments: javac [options] file.java
    int handle_javac_args(int argc, char** argv) {
        vector<string> files;
        string classpath;
        string output_dir = ".";
        vector<string> flags;

        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];

            if (arg == "-cp" || arg == "-classpath") {
                if (i + 1 < argc) {
                    classpath = argv[++i];
                }
            } else if (arg == "-d" && i + 1 < argc) {
                output_dir = argv[++i];
            } else if (arg.substr(0, 1) != "-") {
                files.push_back(arg);
            } else {
                flags.push_back(arg);
            }
        }

        if (files.empty()) {
            cerr << "javac: no input files" << endl;
            return 1;
        }

        for (const auto& file : files) {
            string ext = fs::path(file).extension().string();
            if (ext != ".java") {
                cerr << "javac: not a Java file: " << file << endl;
                continue;
            }

            string output = output_dir + "/" + file.substr(0, file.find_last_of('.')) + ".class";

            JavaProcessor processor;
            processor.registry = registry;
            int ret = processor.compile(file, output, flags, false);

            if (ret != 0) {
                return ret;
            }
        }

        return 0;
    }

    // RustC-style arguments: rustc [options] file.rs
    int handle_rustc_args(int argc, char** argv) {
        vector<string> files;
        string output;
        string out_dir = ".";
        vector<string> flags;
        bool verbose = false;

        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];

            if (arg == "-o" && i + 1 < argc) {
                output = argv[++i];
            } else if (arg == "--out-dir" && i + 1 < argc) {
                out_dir = argv[++i];
            } else if (arg == "-O" || arg == "-g") {
                flags.push_back(arg);
            } else if (arg.substr(0, 1) != "-") {
                files.push_back(arg);
            }
        }

        if (files.empty()) {
            cerr << "rustc: no input files" << endl;
            return 1;
        }

        for (const auto& file : files) {
            string ext = fs::path(file).extension().string();
            if (ext != ".rs") {
                cerr << "rustc: not a Rust file: " << file << endl;
                continue;
            }

            string actual_output = output.empty() ?
                out_dir + "/" + file.substr(0, file.find_last_of('.')) + ".exe" : output;

            RustProcessor processor;
            int ret = processor.compile(file, actual_output, flags, verbose);

            if (ret != 0) {
                return ret;
            }
        }

        return 0;
    }

    // Go-style arguments: go build/run [options] file.go
    int handle_go_args(int argc, char** argv) {
        if (argc < 2) return -1;

        string subcommand = argv[1];
        vector<string> files;
        string output;
        vector<string> flags;

        for (int i = 2; i < argc; ++i) {
            string arg = argv[i];

            if (arg == "-o" && i + 1 < argc) {
                output = argv[++i];
            } else if (arg.substr(0, 1) == "-") {
                flags.push_back(arg);
            } else {
                files.push_back(arg);
            }
        }

        if (files.empty()) {
            cerr << "go: no input files" << endl;
            return 1;
        }

        for (const auto& file : files) {
            string ext = fs::path(file).extension().string();
            if (ext != ".go") {
                cerr << "go: not a Go file: " << file << endl;
                continue;
            }

            string actual_output = output.empty() ?
                file.substr(0, file.find_last_of('.')) + ".exe" : output;

            GoProcessor processor;
            int ret;

            if (subcommand == "run") {
                ret = processor.compile(file, actual_output, flags, false);
                if (ret == 0) {
                    ret = processor.run(actual_output, false);
                }
            } else { // build or default
                ret = processor.compile(file, actual_output, flags, false);
            }

            if (ret != 0) {
                return ret;
            }
        }

        return 0;
    }

    // Python-style arguments: python [options] file.py
    int handle_python_args(int argc, char** argv) {
        vector<string> args;

        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }

        if (args.empty()) {
            // Start Python REPL
            return system("python");
        }

        string file = args.back();
        string ext = fs::path(file).extension().string();

        if (ext != ".py") {
            // Not a Python file, pass through to python
            string cmd = "python";
            for (const auto& arg : args) {
                cmd += " \"" + arg + "\"";
            }
            return system(cmd.c_str());
        }

        PythonProcessor processor;
        return processor.run(file, false);
    }
};

// ============================================================================
// BUILD SYSTEM - Replaces CMake, Make, etc.
// ============================================================================

class BuildSystem {
public:
    CompilerRegistry* registry;
    std::map<std::string, unique_ptr<LanguageProcessor>> processors;

    BuildSystem(CompilerRegistry* reg) : registry(reg) {
        // Register processors
        processors["cpp"] = make_unique<CppProcessor>();
        auto* cpp_processor = static_cast<CppProcessor*>(processors["cpp"].get());
        cpp_processor->registry = registry;

        processors["java"] = make_unique<JavaProcessor>();
        auto* java_processor = static_cast<JavaProcessor*>(processors["java"].get());
        java_processor->registry = registry;

        processors["python"] = make_unique<PythonProcessor>();
        auto* python_processor = static_cast<PythonProcessor*>(processors["python"].get());
        python_processor->registry = registry;

        processors["javascript"] = make_unique<JSProcessor>();
        auto* js_processor = static_cast<JSProcessor*>(processors["javascript"].get());
        js_processor->registry = registry;
        processors["go"] = make_unique<GoProcessor>();
        auto* go_processor = static_cast<GoProcessor*>(processors["go"].get());
        go_processor->registry = registry;
        processors["rust"] = make_unique<RustProcessor>();

        processors["chinese"] = make_unique<ChineseProcessor>();
        auto* chinese_processor = static_cast<ChineseProcessor*>(processors["chinese"].get());
        chinese_processor->registry = registry;
    }

    int build_project(const std::string& project_dir, bool verbose) {
        if (verbose) std::cout << "Building project in: " << project_dir << std::endl;

        // Find all source files
        std::vector<std::string> source_files;
        for (const auto& entry : fs::recursive_directory_iterator(project_dir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (is_supported_extension(ext)) {
                    source_files.push_back(entry.path().string());
                }
            }
        }

        if (source_files.empty()) {
            std::cerr << "No supported source files found" << std::endl;
            return 1;
        }

        // Group by language
        std::map<std::string, std::vector<std::string>> files_by_lang;
        for (const auto& file : source_files) {
            std::string ext = fs::path(file).extension().string();
            files_by_lang[ext].push_back(file);
        }

        // Compile each group
        std::vector<std::string> outputs;
        for (const auto& [ext, files] : files_by_lang) {
            for (const auto& file : files) {
                std::string output = get_processor_by_ext(ext)->get_default_output(file);
                if (get_processor_by_ext(ext)->compile(file, output, {}, verbose) == 0) {
                    outputs.push_back(output);
                    if (verbose) std::cout << "Compiled: " << file << " -> " << output << std::endl;
                } else {
                    std::cerr << "Failed to compile: " << file << std::endl;
                    return 1;
                }
            }
        }

        if (verbose) std::cout << "Build completed successfully!" << std::endl;
        return 0;
    }

    int compile_file(const std::string& input, const std::string& output, bool verbose, bool run_after) {
        std::string ext = fs::path(input).extension().string();
        auto processor = get_processor_by_ext(ext);
        if (!processor) {
            std::cerr << "Unsupported file type: " << ext << std::endl;
            return 1;
        }

        std::string actual_output = output.empty() ? processor->get_default_output(input) : output;

        int ret = processor->compile(input, actual_output, {}, verbose);
        if (ret == 0 && run_after) {
            return processor->run(actual_output, verbose);
        }
        return ret;
    }

private:
    LanguageProcessor* get_processor_by_ext(const std::string& ext) {
        for (auto& [name, processor] : processors) {
            if (processor->can_handle(ext)) {
                return processor.get();
            }
        }
        return nullptr;
    }

    bool is_supported_extension(const std::string& ext) {
        return get_processor_by_ext(ext) != nullptr;
    }

private:
    std::string get_language_from_extension(const std::string& ext) {
        if (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx") return "cpp";
        if (ext == ".java") return "java";
        if (ext == ".py") return "python";
        if (ext == ".js") return "javascript";
        if (ext == ".go") return "go";
        if (ext == ".rs") return "rust";
        if (ext == ".zh") return "chinese";
        return "unknown";
    }

    std::string get_output_path(const std::string& input, const std::string& lang) {
        std::string base = input.substr(0, input.find_last_of('.'));
        if (lang == "cpp") return base + ".exe";
        if (lang == "java") return base + ".class";
        if (lang == "python") return input;
        if (lang == "javascript") return base + ".exe";
        if (lang == "go") return base + ".exe";
        if (lang == "rust") return base + ".exe";
        if (lang == "chinese") return base + ".exe";
        return base + ".out";
    }

    std::vector<std::string> get_default_flags(const std::string& lang) {
        if (lang == "cpp") return {"-Wall", "-std=c++17"};
        if (lang == "java") return {};
        if (lang == "python") return {};
        if (lang == "go") return {};
        if (lang == "rust") return {};
        if (lang == "chinese") return {};
        return {};
    }
};

// ============================================================================
// MAIN APPLICATION
// ============================================================================

// ---- Forward declarations for functions used by main/clean_project ----
int initialize_project(bool verbose);
int list_compilers(const CompilerRegistry& registry, bool verbose);
int clean_project(bool verbose);
bool matches_pattern(const std::string& filename, const std::string& pattern);

int main(int argc, char** argv) {
    初始化中文環境();

    // 在 main() 進入點最前面加：
    if (selfhost::maybe_run_embedded_payload()) {
        return 0; // 若檔尾有 payload，已執行並 ExitProcess()；保險起見 return
    }

    // Check for --help first
    if (argc >= 2 && std::string(argv[1]) == "--help") {
        std::cout << "zhcl - Universal Compiler & Build System Replacement v1.0" << std::endl;
        std::cout << "One CPP file to compile: C, C++, Java, Python, Go, Rust, JS/TS, and more" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: zhcl <command> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  <file>          Compile single file" << std::endl;
        std::cout << "  build           Build entire project" << std::endl;
        std::cout << "  run <file>      Compile and run file" << std::endl;
        std::cout << "  init            Initialize new project" << std::endl;
        std::cout << "  list            List available compilers" << std::endl;
        std::cout << "  clean           Clean build artifacts" << std::endl;
        std::cout << "  selfhost        Self-contained executable generation" << std::endl;
        std::cout << std::endl;
        std::cout << "Selfhost Commands:" << std::endl;
        std::cout << "  selfhost pack <input.(js|py|go|java|zh)> -o <output.exe>    Pack source into self-contained exe" << std::endl;
        std::cout << "  selfhost verify <exe>                                      Verify exe integrity" << std::endl;
        std::cout << "  selfhost explain <input.(js|py|go|java|zh)>                Show bytecode disassembly" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -o <output>     Output file" << std::endl;
        std::cout << "  --verbose       Verbose output" << std::endl;
        std::cout << "  --selfhost      Generate self-contained executable (compile cmd)" << std::endl;
        std::cout << "  --help          Show help" << std::endl;
        std::cout << std::endl;
        std::cout << "Environment Variables:" << std::endl;
        std::cout << "  ZHCL_SELFHOST_QUIET=1    Suppress selfhost banner when running packed executables" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  zhcl hello.c                                    # Compile C file" << std::endl;
        std::cout << "  zhcl hello.java -o hello.class                  # Compile Java file" << std::endl;
        std::cout << "  zhcl selfhost pack hello.js -o hello.exe        # Create self-contained exe" << std::endl;
        std::cout << "  zhcl selfhost verify hello.exe                   # Verify exe integrity" << std::endl;
        std::cout << "  set ZHCL_SELFHOST_QUIET=1 && hello.exe          # Run exe quietly" << std::endl;
        std::cout << std::endl;
        std::cout << "Supported: C/C++, Java, Python, Go, Rust, JavaScript, TypeScript, Chinese (.zh)" << std::endl;
        std::cout << "Replaces: cl, gcc, g++, javac, cmake, make, and traditional build systems" << std::endl;
        return 0;
    }

    CompilerRegistry registry;
    registry.detect_compilers();

    BuildSystem build_system(&registry);

    if (argc < 2) {
        std::cout << "zhcl - Universal Compiler & Build System Replacement v1.0" << std::endl;
        std::cout << "One CPP file to compile: C, C++, Java, Python, Go, Rust, JS/TS, and more" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: zhcl <command> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  <file>          Compile single file" << std::endl;
        std::cout << "  build           Build entire project" << std::endl;
        std::cout << "  run <file>      Compile and run file" << std::endl;
        std::cout << "  init            Initialize new project" << std::endl;
        std::cout << "  list            List available compilers" << std::endl;
        std::cout << "  clean           Clean build artifacts" << std::endl;
        std::cout << "  selfhost        Self-contained executable generation" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -o <output>     Output file" << std::endl;
        std::cout << "  --verbose       Verbose output" << std::endl;
        std::cout << "  --selfhost      Generate self-contained executable (compile cmd)" << std::endl;
        std::cout << "  --help          Show help" << std::endl;
        std::cout << std::endl;
        std::cout << "Supported: C/C++, Java, Python, Go, Rust, JavaScript, TypeScript, Chinese (.zh)" << std::endl;
        std::cout << "Replaces: cl, gcc, g++, javac, cmake, make, and traditional build systems" << std::endl;
        return 0;
    }

    std::string command = argv[1];
    bool verbose = false;
    std::string output;

    // Parse global options
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "-o" && i + 1 < argc) {
            output = argv[++i];
        }
    }

    // Handle commands
    if (command == "build") {
        return build_system.build_project(".", verbose);
    } else if (command == "init") {
        return initialize_project(verbose);
    } else if (command == "list") {
        return list_compilers(registry, verbose);
    } else if (command == "clean") {
        return clean_project(verbose);
    } else if (command == "selfhost") {
        if (argc < 3) {
            std::puts("Usage:\n  zhcl_universal selfhost pack <input.(js|py|go|java|zh)> -o <output.exe>\n"
                      "  zhcl_universal selfhost verify <exe>\n"
                      "  zhcl_universal selfhost explain <input.(js|py|go|java|zh)>\n"
                      "\n"
                      "Note: Generated executables run silently by default. Use --prove/--proof/--selfhost-info\n"
                      "      or set ZHCL_SELFHOST_SHOW=1 to display selfhost verification details.");
            return 1;
        }
        std::string sub = argv[2];
        if (sub == "pack") {
            if (argc < 6 || std::string(argv[4]) != "-o") {
                std::puts("Usage:\n  zhcl_universal selfhost pack <input.(js|py|go|java|zh)> -o <output.exe>");
                return 2;
            }
            std::filesystem::path in = argv[3];
            std::filesystem::path out = argv[5];
            std::string lang;
            auto ext = in.extension().string();
            if (ext == ".js") lang = "js";
            else if (ext == ".py") lang = "py";
            else if (ext == ".go") lang = "go";
            else if (ext == ".java") lang = "java";
            else if (ext == ".zh") lang = "zh";
            else { std::fprintf(stderr, "[selfhost] unsupported input: %s\n", ext.c_str()); return 2; }
            int rc = selfhost::pack_from_file(lang, in, out);
            return rc;
        } else if (sub == "verify") {
            if (argc < 4) { std::puts("Usage:\n  zhcl_universal selfhost verify <exe>"); return 2; }
            return selfhost::verify_exe(argv[3]); // <== New
        } else if (sub == "explain") {
            return selfhost::handle_selfhost_explain(argc, argv);
        }
        std::fprintf(stderr, "Unknown subcommand: selfhost %s\n", sub.c_str());
        return 1;
    } else if (command == "compile") {
        if (argc < 3) {
            std::cerr << "Usage: zhcl compile <file>" << std::endl;
            return 1;
        }
        std::string file = argv[2];
        bool opt_selfhost = false;
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "--selfhost") opt_selfhost = true;
        }
        // ... 判斷輸入副檔名：
        std::filesystem::path input_path(file);
        auto ext = input_path.extension().string();
        if (opt_selfhost && (ext == ".js" || ext == ".zh")) {
            std::string lang = (ext == ".js") ? "js" : "zh";
            std::filesystem::path output_exe = output.empty() ? 
                input_path.parent_path() / (input_path.stem().string() + ".exe") : 
                std::filesystem::path(output);
            int rc = selfhost::pack_from_file(lang, input_path, output_exe);
            if (rc == 0) std::printf("[selfhost] packed -> %s\n", output_exe.string().c_str());
            return rc;
        }
        return build_system.compile_file(file, output, verbose, false);
    } else if (command == "run") {
        if (argc < 3) {
            std::cerr << "Usage: zhcl run <file>" << std::endl;
            return 1;
        }
        std::string file = argv[2];
        return build_system.compile_file(file, output, verbose, true);
    } else {
        // Assume it's a file to compile
        return build_system.compile_file(command, output, verbose, false);
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

int initialize_project(bool verbose) {
    if (verbose) std::cout << "Initializing new zhcl project..." << std::endl;

    // Create zhcl.toml
    ofstream config("zhcl.toml");
    config << "[project]\n";
    config << "name = \"my_project\"\n";
    config << "version = \"0.1.0\"\n";
    config << "type = \"exe\"\n";
    config << "\n[build]\n";
    config << "compiler = \"auto\"\n";
    config << "flags = []\n";
    config << "\n[dependencies]\n";
    config << "# Add dependencies here\n";
    config.close();

    // Create main.cpp
    ofstream main_cpp("main.cpp");
    main_cpp << "#include <iostream>\n";
    main_cpp << "\n";
    main_cpp << "int main() {\n";
    main_cpp << "    std::cout << \"Hello, zhcl!\" << std::endl;\n";
    main_cpp << "    return 0;\n";
    main_cpp << "}\n";
    main_cpp.close();

    // Create README.md
    ofstream readme("README.md");
    readme << "# My zhcl Project\n";
    readme << "\n";
    readme << "Built with zhcl - the universal compiler.\n";
    readme << "\n";
    readme << "## Building\n";
    readme << "```\n";
    readme << "zhcl build\n";
    readme << "```\n";
    readme << "\n";
    readme << "## Running\n";
    readme << "```\n";
    readme << "zhcl run main.cpp\n";
    readme << "```\n";
    readme.close();

    if (verbose) std::cout << "Created zhcl.toml, main.cpp, and README.md" << std::endl;
    return 0;
}

int list_compilers(const CompilerRegistry& registry, bool verbose) {
    // Only show built-in supported languages (no external dependencies needed)
    std::cout << "Built-in supported languages (no external dependencies):" << std::endl;
    std::cout << "  內建 C/C++ (via MSVC/gcc detection)" << std::endl;
    std::cout << "  內建 Java (bytecode generation)" << std::endl;
    std::cout << "  內建 Go (translation to C++)" << std::endl;
    std::cout << "  內建 Chinese (.zh files)" << std::endl;
    std::cout << "  內建 Python (translation to C++)" << std::endl;
    std::cout << "  內建 JavaScript (translation to C++)" << std::endl;

    return 0;
}

int clean_project(bool verbose) {
    if (verbose) std::cout << "Cleaning build artifacts..." << std::endl;

    std::vector<std::string> patterns = {"*.exe", "*.obj", "*.o", "*.class", "*.pyc", "__pycache__"};

    for (const auto& pattern : patterns) {
        for (const auto& entry : fs::recursive_directory_iterator(".")) {
            std::string filename = entry.path().filename().string();
            if (entry.is_regular_file()) {
                if (matches_pattern(filename, pattern)) {
                    fs::remove(entry.path());
                    if (verbose) std::cout << "Removed: " << entry.path() << std::endl;
                }
            } else if (entry.is_directory() && filename == "__pycache__") {
                fs::remove_all(entry.path());
                if (verbose) std::cout << "Removed directory: " << entry.path() << std::endl;
            }
        }
    }

    return 0;
}

bool matches_pattern(const std::string& filename, const std::string& pattern) {
    if (pattern == "*.exe") return filename.find(".exe") != std::string::npos;
    if (pattern == "*.obj") return filename.find(".obj") != std::string::npos;
    if (pattern == "*.o") return filename.find(".o") != std::string::npos;
    if (pattern == "*.class") return filename.find(".class") != std::string::npos;
    if (pattern == "*.pyc") return filename.find(".pyc") != std::string::npos;
    return false;
}
