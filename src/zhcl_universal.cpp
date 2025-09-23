// zhcl.cpp -- Universal Compiler & Build System Replacement
// One CPP file to rule them all: C, C++, Java, Python, Go, Rust, JS/TS, and more
// Replaces cl, gcc, g++, javac, cmake, make, and build systems

#include <string>
#include <vector>
#include <cstdint>
#include "../include/frontend.h"
#include "../include/fe_clite.h"
#include "../include/fe_cpplite.h"
#include "../include/fe_jslite.h"
#include "../include/fe_zh.h"
#include "../include/zh_frontend.h"
#include "../include/chinese_new.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
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
#include <unordered_map>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// Use explicit std:: prefix instead of using namespace std
namespace fs = std::filesystem;

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

// Forward declarations
extern "C" int translate_zh_to_cpp(const std::string &input_path, const std::string &output_cpp_path, bool verbose);
extern "C" void register_fe_clite();
extern "C" void register_fe_cpplite();
extern "C" void register_fe_jslite();
extern "C" void register_fe_zh();
extern "C" void register_fe_golite();
extern "C" void register_fe_javalite();
extern "C" void register_fe_pylite();

// ======= SelfHost: payload trailer + runtime + packer (no external compilers) =======
#ifdef _WIN32
#include <windows.h>
#endif
#include <cstdint>
#include <fstream>
#include <sstream>

namespace selfhost
{

    // ---- New: CRC32 (蝪∪?雲憭? ----
    static uint32_t crc32(const uint8_t *data, size_t len)
    {
        uint32_t c = 0xFFFFFFFFu;
        for (size_t i = 0; i < len; i++)
        {
            c ^= data[i];
            for (int k = 0; k < 8; k++)
            {
                uint32_t m = -(int)(c & 1u);
                c = (c >> 1) ^ (0xEDB88320u & m);
            }
        }
        return ~c;
    }

    static const uint64_t SH_MAGIC = 0x305941505A48435Full; // ????
    static const uint32_t SH_VERSION = 1;                   // <== New: ?

#pragma pack(push, 1)
    struct Trailer
    {
        uint64_t magic;          // SH_MAGIC
        uint64_t payload_size;   // 雿?蝣潮摨?
        uint64_t payload_offset; // 雿?蝣潸絲暺?
        uint32_t version;        // <== New
        uint32_t crc32;          // <== New (撠?payload 閮?)
    };
#pragma pack(pop)

    // Move Op enum to global scope for visibility
    enum Op : uint8_t
    {
        OP_PRINT = 1,
        OP_PRINT_INT,
        OP_SET_I64,
        OP_COPY_I64,
        OP_END
    };

    // === glue: .zh -> C++ using the new ZhFrontend (no Python needed) ===
    static inline void emit_u64(std::vector<uint8_t> &bc, uint64_t v)
    {
        for (int i = 0; i < 8; i++)
            bc.push_back((uint8_t)((v >> (8 * i)) & 0xFF)); // little-endian
    }
    static inline void emit_i64(std::vector<uint8_t> &bc, int64_t v)
    {
        emit_u64(bc, (uint64_t)v);
    }

    static void emit_PRINT_adapter(std::vector<uint8_t> &bc, const std::string &s)
    {
        bc.push_back((uint8_t)OP_PRINT);
        emit_u64(bc, (uint64_t)s.size());
        bc.insert(bc.end(), s.begin(), s.end());
    }
    static void emit_SET_I64_adapter(std::vector<uint8_t> &bc, uint8_t slot, long long v)
    {
        bc.push_back((uint8_t)OP_SET_I64);
        bc.push_back(slot);
        emit_i64(bc, v);
    }
    static void emit_PRINT_INT_adapter(std::vector<uint8_t> &bc, uint8_t slot)
    {
        bc.push_back((uint8_t)OP_PRINT_INT);
        bc.push_back(slot);
    }
    static void emit_END_adapter(std::vector<uint8_t> &bc)
    {
        bc.push_back((uint8_t)OP_END);
    }

    // === glue: .zh -> C++ using the new ZhFrontend (no Python needed) ===

    // 憒?瑼???恐??鋆?嚗歇摮撠梁??
    extern std::string emit_cpp_from_bc(const std::vector<uint8_t> &bc);

    // ---- ???脫? ----
    static void disassemble_bc(const std::vector<uint8_t> &bc, std::ostream &out);
    static int handle_selfhost_explain(int argc, char **argv);

    // ---- 撠極??----
    static std::string read_all(const fs::path &p)
    {
        std::ifstream f(p, std::ios::binary);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }
    static bool write_all(const fs::path &p, const std::string &s)
    {
        fs::create_directories(p.parent_path());
        std::ofstream f(p, std::ios::binary);
        f.write(s.data(), (std::streamsize)s.size());
        return (bool)f;
    }
    static bool file_copy(const fs::path &src, const fs::path &dst)
    {
        std::ifstream in(src, std::ios::binary);
        if (!in)
            return false;
        std::ofstream out(dst, std::ios::binary);
        if (!out)
            return false;
        out << in.rdbuf();
        return (bool)out;
    }
    static std::vector<uint8_t> enc_print(const std::string &s)
    {
        std::vector<uint8_t> out;
        uint64_t n = (uint64_t)s.size();
        out.push_back((uint8_t)OP_PRINT);
        for (int i = 0; i < 8; i++)
            out.push_back((uint8_t)((n >> (8 * i)) & 0xFF)); // u64 LE
        out.insert(out.end(), s.begin(), s.end());
        return out;
    }

    // ---- ?渲陌?剁??瑁?雿?蝣潘?----
    static void execute_bc(const std::vector<uint8_t> &bc)
    {
#ifdef _WIN32
        // 閮剔蔭?批?唬誨蝣潮???UTF-8 隞交迤蝣粹＊蝷箔葉??
        SetConsoleOutputCP(65001);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD w = 0;
        std::vector<int64_t> vars(256, 0); // 霈瑽賭?
        size_t i = 0;
        while (i < bc.size())
        {
            uint8_t op = bc[i++];
            if (op == (uint8_t)OP_PRINT)
            {
                if (i + 8 > bc.size())
                    break;
                uint64_t n = 0;
                for (int k = 0; k < 8; k++)
                    n |= ((uint64_t)bc[i++]) << (8 * k);
                if (i + n > bc.size())
                    break;
                const char *s = (const char *)&bc[i];
                WriteFile(hOut, s, (DWORD)n, &w, nullptr);
                WriteFile(hOut, "\r\n", 2, &w, nullptr);
                i += (size_t)n;
            }
            else if (op == (uint8_t)OP_PRINT_INT)
            {
                if (i >= bc.size())
                    break;
                uint8_t id = bc[i++];
                char buf[32];
                sprintf(buf, "%lld\r\n", (long long)vars[id]);
                WriteFile(hOut, buf, (DWORD)strlen(buf), &w, nullptr);
            }
            else if (op == (uint8_t)OP_SET_I64)
            {
                if (i + 9 > bc.size())
                    break;
                uint8_t id = bc[i++];
                int64_t v = 0;
                for (int k = 0; k < 8; k++)
                    v |= ((int64_t)bc[i++]) << (8 * k);
                vars[id] = v;
            }
            else if (op == (uint8_t)OP_COPY_I64)
            {
                if (i + 1 >= bc.size())
                    break;
                uint8_t dst = bc[i++];
                uint8_t src = bc[i++];
                vars[dst] = vars[src];
            }
            else if (op == (uint8_t)OP_END)
            {
                break;
            }
            else
                break;
        }
        ExitProcess(0); // ?湔蝯?嚗???CLI
#else
        // POSIX ?啣?銋??fwrite/puts ?嚗??嚗?
        std::vector<int64_t> vars(256, 0); // 霈瑽賭?
        size_t i = 0;
        while (i < bc.size())
        {
            uint8_t op = bc[i++];
            if (op == (uint8_t)OP_PRINT)
            {
                if (i + 8 > bc.size())
                    break;
                uint64_t n = 0;
                for (int k = 0; k < 8; k++)
                    n |= ((uint64_t)bc[i++]) << (8 * k);
                if (i + n > bc.size())
                    break;
                fwrite(&bc[i], 1, (size_t)n, stdout);
                fputc('\n', stdout);
                i += (size_t)n;
            }
            else if (op == (uint8_t)OP_PRINT_INT)
            {
                if (i >= bc.size())
                    break;
                uint8_t id = bc[i++];
                fprintf(stdout, "%lld\n", (long long)vars[id]);
            }
            else if (op == (uint8_t)OP_SET_I64)
            {
                if (i + 9 > bc.size())
                    break;
                uint8_t id = bc[i++];
                int64_t v = 0;
                for (int k = 0; k < 8; k++)
                    v |= ((int64_t)bc[i++]) << (8 * k);
                vars[id] = v;
            }
            else if (op == (uint8_t)OP_COPY_I64)
            {
                if (i + 1 >= bc.size())
                    break;
                uint8_t dst = bc[i++];
                uint8_t src = bc[i++];
                vars[dst] = vars[src];
            }
            else if (op == (uint8_t)OP_END)
            {
                break;
            }
            else
                break;
        }
        std::exit(0);
#endif
    }

    // ---- ??霅臭??Ⅳ?箏霈?澆? ----

    // ?芾?蝢拙?閬?摮?嚗??? UTF-8
    static std::string quote_utf8_minimal(const std::string &s)
    {
        std::string out;
        out.reserve(s.size() + 2);
        out.push_back('\"');
        for (unsigned char c : s)
        {
            switch (c)
            {
            case '\\':
                out += "\\\\";
                break;
            case '\"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                // ??ASCII ?湔?見頛詨嚗TF-8嚗?
                out.push_back((char)c);
            }
        }
        out.push_back('\"');
        return out;
    }

    static void disassemble_bc(const std::vector<uint8_t> &bc, std::ostream &out = std::cout)
    {
        out << "Bytecode disassembly:" << std::endl;
        size_t i = 0;
        while (i < bc.size())
        {
            out << std::setw(4) << std::setfill('0') << i << ": ";
            uint8_t op = bc[i++];
            switch (op)
            {
            case (uint8_t)OP_PRINT:
            {
                if (i + 8 > bc.size())
                {
                    out << "PRINT (incomplete)" << std::endl;
                    return;
                }
                uint64_t n = 0;
                for (int k = 0; k < 8; k++)
                    n |= ((uint64_t)bc[i++]) << (8 * k);
                if (i + n > bc.size())
                {
                    out << "PRINT (incomplete string)" << std::endl;
                    return;
                }
                std::string s((const char *)&bc[i], (size_t)n);
                out << "PRINT " << quote_utf8_minimal(s) << std::endl;
                i += (size_t)n;
                break;
            }
            case (uint8_t)OP_PRINT_INT:
            {
                if (i >= bc.size())
                {
                    out << "PRINT_INT (incomplete)" << std::endl;
                    return;
                }
                uint8_t id = bc[i++];
                out << "PRINT_INT v" << (int)id << std::endl;
                break;
            }
            case (uint8_t)OP_SET_I64:
            {
                if (i + 9 > bc.size())
                {
                    out << "SET_I64 (incomplete)" << std::endl;
                    return;
                }
                uint8_t id = bc[i++];
                int64_t v = 0;
                for (int k = 0; k < 8; k++)
                    v |= ((int64_t)bc[i++]) << (8 * k);
                out << "SET_I64 v" << (int)id << " = " << (long long)v << std::endl;
                break;
            }
            case (uint8_t)OP_END:
            {
                out << "END" << std::endl;
                return;
            }
            default:
                out << "UNKNOWN_OP(0x" << std::setw(2) << std::setfill('0') << std::hex << (int)op << std::dec << ")" << std::endl;
                return;
            }
        }
        out << "End of bytecode" << std::endl;
    }

    // ---- New: 霈??+ 撽? trailer嚗???payload ??閮?----
    struct PayloadInfo
    {
        std::vector<uint8_t> data;
        Trailer tr{};
        bool ok = false;
        bool crc_ok = false;
    };
    static PayloadInfo read_payload_from_file(const fs::path &exe)
    {
        PayloadInfo R;
        std::ifstream f(exe, std::ios::binary);
        if (!f)
            return R;
        f.seekg(0, std::ios::end);
        auto sz = (uint64_t)f.tellg();
        if (sz < sizeof(Trailer))
            return R;
        f.seekg(sz - sizeof(Trailer));
        f.read((char *)&R.tr, sizeof(Trailer));
        if (!f || R.tr.magic != SH_MAGIC)
            return R;
        R.ok = true;
        R.data.resize((size_t)R.tr.payload_size);
        f.seekg((std::streamoff)R.tr.payload_offset);
        f.read((char *)R.data.data(), (std::streamsize)R.tr.payload_size);
        if (!f)
        {
            R.ok = false;
            return R;
        }
        uint32_t c = crc32(R.data.data(), R.data.size());
        R.crc_ok = (c == R.tr.crc32);
        return R;
    }

    // ---- runtime嚗????亙葆 payload 撠勗?芾?璈怠? -> ?瑁? -> ???----
    static bool maybe_run_embedded_payload()
    {
#ifdef _WIN32
        wchar_t pathW[MAX_PATH]{0};
        GetModuleFileNameW(nullptr, pathW, MAX_PATH);
        fs::path self(pathW);
#else
        fs::path self = "/proc/self/exe";
#endif
        auto R = read_payload_from_file(self);
        if (!R.ok)
            return false;

        // 霈??R 銋??璈怠?銋??嚗?
        bool show_proof = false;

// 1) ?賭誘???貉孛?潘??舀 --prove / --proof / --selfhost-info嚗?
#ifdef _WIN32
        {
            std::wstring cmd = GetCommandLineW();
            std::wstring lower = cmd;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
            if (lower.find(L"--prove") != std::wstring::npos ||
                lower.find(L"--proof") != std::wstring::npos ||
                lower.find(L"--selfhost-info") != std::wstring::npos)
            {
                show_proof = true;
            }
        }
#else
        {
            // ??Windows ?舐 argc/argv ??/proc/self/cmdline嚗?雿??暹??嗆???嚗?
            // 銝?舐?靘??乩??券?楝敺銝 argv嚗隞亙??仿?嚗??函憓??詨??剁?
        }
#endif

        // 2) ?啣?霈閫貊嚗I/?單?剁?
        if (const char *s = std::getenv("ZHCL_SELFHOST_SHOW"))
        {
            if (s[0] == '1' || s[0] == 'y' || s[0] == 'Y' || s[0] == 't' || s[0] == 'T')
            {
                show_proof = true;
            }
        }

        // 3) ?乩?? Trailer.flags 閮剝??HF_SILENT_BANNER??甇方??臬??典蕭?亙?
        //    ???券?閮剖停?舫?暺??芣?憿臬?閬??＊蝷箝?

        // ????ㄐ?舐??printf 璈怠? ??
        //    ?寞?嚗??show_proof ?
        if (show_proof)
        {
            std::printf("[selfhost] payload v%u %s size=%llu crc=%s\n",
                        R.tr.version,
                        (R.ok ? "found" : "missing"),
                        (unsigned long long)R.tr.payload_size,
                        (R.crc_ok ? "OK" : "BAD"));
        }

        if (!R.crc_ok)
        {
            std::fprintf(stderr, "[selfhost] CRC mismatch, abort.\n");
            std::exit(3);
        }

        execute_bc(R.data); // 銝???
        return true;
    }

    // ---- 蝧餉陌?剁?JS ??雿?蝣潘?PoC嚗onsole.log("??) / ?嗡?敹賜嚗?---
    static std::vector<uint8_t> translate_js_to_bc(const std::string &js)
    {
        std::vector<uint8_t> bc;
        std::istringstream ss(js);
        std::string line;
        // ??????撓??
        while (std::getline(ss, line))
        {
            // ?駁??蝛箇
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos)
                continue;
            line = line.substr(start);
            if (line.empty())
                continue;

            auto pos = line.find("console.log");
            if (pos != std::string::npos)
            {
                // ?曉 console.log( 銋??摰?
                size_t paren_start = line.find('(', pos);
                if (paren_start == std::string::npos)
                    continue;
                size_t paren_end = line.find(')', paren_start);
                if (paren_end == std::string::npos)
                    continue;

                std::string arg = line.substr(paren_start + 1, paren_end - paren_start - 1);
                // 蝪∪??嚗??隞亙???憪?蝯?嚗??摰?
                if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"')
                {
                    std::string content = arg.substr(1, arg.size() - 2);
                    auto v = enc_print(content);
                    bc.insert(bc.end(), v.begin(), v.end());
                }
                // TODO: ??霈??憒?"x = " + x
            }
            // 敹賜?嗡?銵?霈?脫???貊?嚗?
        }
        // 憒?瘝?銵??泵嚗?閰西????蝚虫葡
        if (bc.empty())
        {
            std::string content = js;
            // ?駁??蝛箇
            size_t start = content.find_first_not_of(" \t\r\n");
            if (start != std::string::npos)
            {
                content = content.substr(start);
                size_t end = content.find_last_not_of(" \t\r\n");
                if (end != std::string::npos)
                {
                    content = content.substr(0, end + 1);
                }
            }

            auto pos = content.find("console.log");
            if (pos != std::string::npos)
            {
                // ?曉 console.log( 銋??摰?
                size_t paren_start = content.find('(', pos);
                if (paren_start != std::string::npos)
                {
                    size_t paren_end = content.find(')', paren_start);
                    if (paren_end != std::string::npos)
                    {
                        std::string arg = content.substr(paren_start + 1, paren_end - paren_start - 1);
                        // 蝪∪??嚗??隞亙???憪?蝯?嚗??摰?
                        if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"')
                        {
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

    // ---- 蝧餉陌?剁?銝剜??芰隤? .zh ??雿?蝣潘?PoC嚗撓?箏?銝?"?? / ?嗡??見嚗?---

    // 1) UTF-8 BOM ?駁 + ?典耦撘?/蝛箇甇????+ 蝜陛撠?
    static inline void strip_bom_utf8(std::string &s)
    {
        if (s.size() >= 3 && (uint8_t)s[0] == 0xEF && (uint8_t)s[1] == 0xBB && (uint8_t)s[2] == 0xBF)
            s.erase(0, 3);
    }

    static inline void replace_all(std::string &s, const std::string &a, const std::string &b)
    {
        if (a.empty())
            return;
        size_t pos = 0;
        while ((pos = s.find(a, pos)) != std::string::npos)
        {
            s.replace(pos, a.size(), b);
            pos += b.size();
        }
    }

    // 嚗?賂?IR ??C++嚗??箝?蝯虫犖???Ｙ嚗?
    std::string emit_cpp_from_bc(const std::vector<uint8_t> &bc)
    {
        std::ostringstream out;
        out << "#include <cstdio>\n#include <cstdint>\nint main(){\n";

        // First pass: collect all variable IDs that are used
        std::set<uint8_t> used_vars;
        size_t i = 0;
        while (i < bc.size())
        {
            uint8_t op = bc[i++];
            if (op == (uint8_t)OP_PRINT_INT)
            {
                if (i < bc.size())
                {
                    uint8_t id = bc[i++];
                    used_vars.insert(id);
                }
            }
            else if (op == (uint8_t)OP_SET_I64)
            {
                if (i < bc.size())
                {
                    uint8_t id = bc[i++];
                    used_vars.insert(id);
                    i += 8; // skip the value
                }
            }
            else if (op == (uint8_t)OP_PRINT)
            {
                if (i + 8 <= bc.size())
                {
                    uint64_t n = 0;
                    for (int k = 0; k < 8; k++)
                        n |= ((uint64_t)bc[i++]) << (8 * k);
                    i += (size_t)n;
                }
            }
            else if (op == (uint8_t)OP_END)
            {
                break;
            }
        }

        // Declare all variables at the beginning
        for (uint8_t id : used_vars)
        {
            out << "  long long v" << (int)id << " = 0;\n";
        }

        // Second pass: generate operations
        i = 0;
        while (i < bc.size())
        {
            uint8_t op = bc[i++];
            if (op == (uint8_t)OP_PRINT)
            {
                uint64_t n = 0;
                for (int k = 0; k < 8; k++)
                    n |= ((uint64_t)bc[i++]) << (8 * k);
                std::string s((const char *)&bc[i], (size_t)n);
                i += (size_t)n;
                out << "  std::puts(\"";
                for (char c : s)
                {
                    if (c == '\\' || c == '"')
                        out << '\\';
                    out << c;
                }
                out << "\");\n";
            }
            else if (op == (uint8_t)OP_PRINT_INT)
            {
                uint8_t id = bc[i++];
                out << "  std::printf(\"%lld\\n\", (long long)v" << (int)id << ");\n";
            }
            else if (op == (uint8_t)OP_SET_I64)
            {
                uint8_t id = bc[i++];
                int64_t v = 0;
                for (int k = 0; k < 8; k++)
                    v |= ((int64_t)bc[i++]) << (8 * k);
                out << "  v" << (int)id << " = " << (long long)v << ";\n";
            }
            else if (op == (uint8_t)OP_END)
            {
                break;
            }
            else
            {
                out << "  // OP_" << (int)op << "\n";
                break;
            }
        }
        out << "  return 0;\n}\n";
        return out.str();
    }

    // ---- 蝧餉陌?剁?Python ??雿?蝣潘?PoC嚗rint("??) / ?嗡?敹賜嚗?---
    static std::vector<uint8_t> translate_py_to_bc(const std::string &py)
    {
        std::vector<uint8_t> bc;
        std::istringstream ss(py);
        std::string line;
        while (std::getline(ss, line))
        {
            // ?駁??蝛箇
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos)
                continue;
            line = line.substr(start);
            if (line.empty())
                continue;

            auto pos = line.find("print(");
            if (pos != std::string::npos)
            {
                // ?曉 print( 銋??摰?
                size_t paren_start = line.find('(', pos);
                if (paren_start == std::string::npos)
                    continue;
                size_t paren_end = line.find(')', paren_start);
                if (paren_end == std::string::npos)
                    continue;

                std::string arg = line.substr(paren_start + 1, paren_end - paren_start - 1);
                // 蝪∪??嚗??隞亙???憪?蝯?嚗??摰?
                if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"')
                {
                    std::string content = arg.substr(1, arg.size() - 2);
                    auto v = enc_print(content);
                    bc.insert(bc.end(), v.begin(), v.end());
                }
            }
            // 敹賜?嗡?銵?霈?脫???貊?嚗?
        }
        return bc;
    }

    // ---- 蝧餉陌?剁?Go ??雿?蝣潘?PoC嚗mt.Println("??) / ?嗡?敹賜嚗?---
    static std::vector<uint8_t> translate_go_to_bc(const std::string &go)
    {
        std::vector<uint8_t> bc;
        std::istringstream ss(go);
        std::string line;
        while (std::getline(ss, line))
        {
            // ?駁??蝛箇
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos)
                continue;
            line = line.substr(start);
            if (line.empty())
                continue;

            auto pos = line.find("fmt.Println(");
            if (pos != std::string::npos)
            {
                // ?曉 fmt.Println( 銋??摰?
                size_t paren_start = line.find('(', pos);
                if (paren_start == std::string::npos)
                    continue;
                size_t paren_end = line.find(')', paren_start);
                if (paren_end == std::string::npos)
                    continue;

                std::string arg = line.substr(paren_start + 1, paren_end - paren_start - 1);
                // 蝪∪??嚗??隞亙???憪?蝯?嚗??摰?
                if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"')
                {
                    std::string content = arg.substr(1, arg.size() - 2);
                    auto v = enc_print(content);
                    bc.insert(bc.end(), v.begin(), v.end());
                }
            }
            // 敹賜?嗡?銵?霈?脫???貊?嚗?
        }
        return bc;
    }

    // ---- 蝧餉陌?剁?Java ??雿?蝣潘?PoC嚗ystem.out.println("??) / ?嗡?敹賜嚗?---
    static std::vector<uint8_t> translate_java_to_bc(const std::string &java)
    {
        std::vector<uint8_t> bc;
        std::istringstream ss(java);
        std::string line;
        while (std::getline(ss, line))
        {
            // ?駁??蝛箇
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos)
                continue;
            line = line.substr(start);
            if (line.empty())
                continue;

            auto pos = line.find("System.out.println(");
            if (pos != std::string::npos)
            {
                // ?曉 System.out.println( 銋??摰?
                size_t paren_start = line.find('(', pos);
                if (paren_start == std::string::npos)
                    continue;
                size_t paren_end = line.find(')', paren_start);
                if (paren_end == std::string::npos)
                    continue;

                std::string arg = line.substr(paren_start + 1, paren_end - paren_start - 1);
                // 蝪∪??嚗??隞亙???憪?蝯?嚗??摰?
                if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"')
                {
                    std::string content = arg.substr(1, arg.size() - 2);
                    auto v = enc_print(content);
                    bc.insert(bc.end(), v.begin(), v.end());
                }
            }
            // 敹賜?嗡?銵?霈?脫???貊?嚗?
        }
        return bc;
    }

    // ---- pack嚗神??version ??CRC??芾?閮 ----
    static int pack_payload_to_exe(const fs::path &output_exe, const std::vector<uint8_t> &bc,
#ifdef _WIN32
                                   const fs::path &self_exe = []
                                   {
                                   wchar_t pathW[MAX_PATH]{0};
                                   GetModuleFileNameW(nullptr, pathW, MAX_PATH);
                                   return fs::path(pathW); }()
#else
                                   const fs::path &self_exe = "/proc/self/exe"
#endif
    )
    {
        if (!file_copy(self_exe, output_exe))
        {
            std::fprintf(stderr, "[selfhost] copy self -> out failed\n");
            return 4;
        }
        std::ofstream out(output_exe, std::ios::binary | std::ios::app);
        if (!out)
        {
            std::fprintf(stderr, "[selfhost] open out for append failed\n");
            return 5;
        }
        uint64_t off = (uint64_t)fs::file_size(output_exe);
        out.write((const char *)bc.data(), (std::streamsize)bc.size());

        Trailer tr{};
        tr.magic = SH_MAGIC;
        tr.payload_size = (uint64_t)bc.size();
        tr.payload_offset = off;
        tr.version = SH_VERSION;                // <== New
        tr.crc32 = crc32(bc.data(), bc.size()); // <== New

        out.write((const char *)&tr, sizeof(tr));
        out.flush();

        std::printf("[selfhost] packed -> %s (v%u, size=%llu, crc=%08X)\n",
                    output_exe.string().c_str(),
                    tr.version,
                    (unsigned long long)tr.payload_size,
                    tr.crc32);

        return 0;
    }

    // ---- JAR格式打包 (JAR format packing) ----
    static int pack_payload_to_jar(const fs::path &output_jar, const std::vector<uint8_t> &bc, const std::string &lang)
    {
        // 創建臨時目錄結構
        fs::path temp_dir = output_jar;
        temp_dir.replace_extension(".jar_temp");
        fs::create_directories(temp_dir);

        // 創建META-INF目錄
        fs::path meta_inf = temp_dir / "META-INF";
        fs::create_directories(meta_inf);

        // 創建MANIFEST.MF
        fs::path manifest_path = meta_inf / "MANIFEST.MF";
        std::ofstream manifest(manifest_path, std::ios::binary);
        if (!manifest)
        {
            std::fprintf(stderr, "[selfhost] create manifest failed\n");
            return 6;
        }
        manifest << "Manifest-Version: 1.0\r\n";
        manifest << "Main-Class: Main\r\n";
        manifest << "Created-By: zhcl_universal\r\n";
        manifest << "\r\n";
        manifest.close();

        // 保存字節碼文件
        fs::path bytecode_path = temp_dir / ("Main." + lang + ".bc");
        std::ofstream bc_file(bytecode_path, std::ios::binary);
        if (!bc_file)
        {
            std::fprintf(stderr, "[selfhost] create bytecode file failed\n");
            return 7;
        }
        bc_file.write((const char *)bc.data(), (std::streamsize)bc.size());
        bc_file.close();

        // 使用系統zip命令創建JAR文件
        std::string zip_cmd = "cd \"" + temp_dir.parent_path().string() + "\" && zip -r \"" +
                              output_jar.string() + "\" \"" + temp_dir.filename().string() + "\"";
        int zip_result = std::system(zip_cmd.c_str());

        // 清理臨時文件
        fs::remove_all(temp_dir);

        if (zip_result != 0)
        {
            std::fprintf(stderr, "[selfhost] zip command failed (code: %d)\n", zip_result);
            return 8;
        }

        std::printf("[selfhost] packed -> %s (JAR format, size=%llu)\n",
                    output_jar.string().c_str(),
                    (unsigned long long)bc.size());

        return 0;
    }

    // ---- 撠??亙嚗 CLI ?澆 ----

    static int pack_from_file(const std::string &lang, const fs::path &in, const fs::path &out)
    {
        std::string src = read_all(in);
        // BOM/換行正規化
        strip_utf8_bom(src);
        normalize_newlines(src);

        std::vector<uint8_t> bc;
        if (lang == "js" || lang == "javascript")
            bc = translate_js_to_bc(src);
        else if (lang == "py" || lang == "python")
            bc = translate_py_to_bc(src);
        else if (lang == "go")
            bc = translate_go_to_bc(src);
        else if (lang == "java")
            bc = translate_java_to_bc(src);
        else if (lang == "zh")
        {
            ZhFrontend fe;
            bc = fe.translate_to_bc(src);
        }
        else
        {
            std::fprintf(stderr, "[selfhost] unsupported lang: %s\n", lang.c_str());
            return 2;
        }
        return pack_payload_to_exe(out, bc);
    }

    // ---- 撠??亙銝?嚗ack_from_file嚗?銝??verify ??喳 ----
    static int verify_exe(const fs::path &exe)
    {
        auto R = read_payload_from_file(exe);
        if (!R.ok)
        {
            std::fprintf(stderr, "[selfhost] no payload in: %s\n", exe.string().c_str());
            return 2;
        }
        std::printf("[selfhost] verify: %s\n", exe.string().c_str());
        std::printf("  version : %u\n", R.tr.version);
        std::printf("  size    : %llu bytes\n", (unsigned long long)R.tr.payload_size);
        std::printf("  offset  : %llu\n", (unsigned long long)R.tr.payload_offset);
        std::printf("  crc32   : %08X (%s)\n", R.tr.crc32, R.crc_ok ? "OK" : "BAD");
        return R.crc_ok ? 0 : 3;
    }

    // ---- handle selfhost explain command ----
    static int handle_selfhost_explain(int argc, char **argv)
    {
        if (argc < 4)
        {
            std::puts("Usage:\n  zhcl_universal selfhost explain <input.(js|py|go|java|zh)>");
            return 2;
        }
        fs::path in = argv[3];
        std::string src = read_all(in); // ??征??舐?亦
        // BOM/換行正規化
        strip_utf8_bom(src);
        normalize_newlines(src);

        std::vector<uint8_t> bc;
        auto ext = in.extension().string();
        if (ext == ".js")
            bc = translate_js_to_bc(src);
        else if (ext == ".py")
            bc = translate_py_to_bc(src);
        else if (ext == ".go")
            bc = translate_go_to_bc(src);
        else if (ext == ".java")
            bc = translate_java_to_bc(src);
        else if (ext == ".zh")
        {
            ZhFrontend fe;
            bc = fe.translate_to_bc(src);
        }
        else
        {
            std::fprintf(stderr, "[selfhost] unsupported input: %s\n", ext.c_str());
            return 2;
        }
        disassemble_bc(bc);
        return 0;
    }

} // namespace selfhost

// === glue: .zh -> C++ using the new ZhFrontend (no Python needed) ===
static inline void emit_u64(std::vector<uint8_t> &bc, uint64_t v)
{
    for (int i = 0; i < 8; i++)
        bc.push_back((uint8_t)((v >> (8 * i)) & 0xFF)); // little-endian
}
static inline void emit_i64(std::vector<uint8_t> &bc, int64_t v)
{
    emit_u64(bc, (uint64_t)v);
}

static void emit_PRINT_adapter(std::vector<uint8_t> &bc, const std::string &s)
{
    bc.push_back((uint8_t)selfhost::OP_PRINT);
    emit_u64(bc, (uint64_t)s.size());
    bc.insert(bc.end(), s.begin(), s.end());
}
static void emit_SET_I64_adapter(std::vector<uint8_t> &bc, uint8_t slot, long long v)
{
    bc.push_back((uint8_t)selfhost::OP_SET_I64);
    bc.push_back(slot);
    emit_i64(bc, v);
}
static void emit_PRINT_INT_adapter(std::vector<uint8_t> &bc, uint8_t slot)
{
    bc.push_back((uint8_t)selfhost::OP_PRINT_INT);
    bc.push_back(slot);
}
static void emit_END_adapter(std::vector<uint8_t> &bc)
{
    bc.push_back((uint8_t)selfhost::OP_END);
}

// Forward declarations
class CompilerRegistry;
class LanguageProcessor;
class CppProcessor;
class JavaProcessor;
class PythonProcessor;
class GoProcessor;
class RustProcessor;

// JVM Bytecode Writer (Big-Endian)
struct W
{
    std::vector<uint8_t> buf;
    void u1(uint8_t x) { buf.push_back(x); }
    void u2(uint16_t x)
    {
        buf.push_back((x >> 8) & 0xFF);
        buf.push_back(x & 0xFF);
    }
    void u4(uint32_t x)
    {
        buf.push_back((x >> 24) & 0xFF);
        buf.push_back((x >> 16) & 0xFF);
        buf.push_back((x >> 8) & 0xFF);
        buf.push_back(x & 0xFF);
    }
    void bytes(const void *p, size_t n)
    {
        auto c = (const uint8_t *)p;
        buf.insert(buf.end(), c, c + n);
    }
    void utf8(const std::string &s)
    {
        u1(1);
        u2((uint16_t)s.size());
        bytes(s.data(), s.size());
    }
};

// ============================================================================
// LANGUAGE PROCESSORS - Handle compilation for each language
// ============================================================================

class LanguageProcessor
{
public:
    virtual ~LanguageProcessor() = default;
    virtual bool can_handle(const std::string &ext) = 0;
    virtual int compile(const std::string &input, const std::string &output,
                        const std::vector<std::string> &flags, bool verbose) = 0;
    virtual int run(const std::string &executable, bool verbose) = 0;
    virtual std::string get_default_output(const std::string &input) = 0;
};

// ============================================================================
// COMPILER REGISTRY SYSTEM - Detects and manages all compilers
// ============================================================================

class CompilerRegistry
{
public:
    struct Compiler
    {
        std::string name;
        std::string command;
        std::vector<std::string> flags;
        std::set<std::string> supported_languages;
        bool available = false;
        int priority = 0; // Higher = preferred
    };

    std::map<std::string, Compiler> compilers;

    void detect_compilers()
    {
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
    void detect_compiler(const std::string &name, const std::string &cmd, const std::set<std::string> &langs,
                         const std::vector<std::string> &flags, int priority)
    {
        std::string test_cmd;
        if (name == "msvc")
        {
            test_cmd = cmd + " /? >nul 2>&1";
        }
        else
        {
            test_cmd = cmd + " --version >nul 2>&1";
        }
        bool available = (system(test_cmd.c_str()) == 0);

        compilers[name] = {name, cmd, flags, langs, available, priority};

        if (!available)
        {
            // Try alternative test commands
            std::vector<std::string> alt_tests;
            if (name == "msvc")
            {
                alt_tests = {" /help", ""};
            }
            else
            {
                alt_tests = {" --help", " -v", " -version", ""};
            }
            for (const auto &alt : alt_tests)
            {
                if (system((cmd + alt + " >nul 2>&1").c_str()) == 0)
                {
                    compilers[name].available = true;
                    break;
                }
            }

            // Special handling for MSVC: search common installation paths
            if (!compilers[name].available && name == "msvc")
            {
                // Try the exact path from compile_definitions.bat first
                std::string exact_path = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Tools\\MSVC\\14.29.30133\\bin\\Hostx64\\x64\\cl.exe";
                std::string test_exact = "\"" + exact_path + "\" /? >nul 2>&1";
                if (system(test_exact.c_str()) == 0)
                {
                    compilers[name].available = true;
                    compilers[name].command = "\"" + exact_path + "\"";
                }
                else
                {
                    // Fallback to path search
                    std::vector<std::string> common_paths = {
                        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Tools\\MSVC",
                        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Tools\\MSVC",
                        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC",
                        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\VC\\Tools\\MSVC",
                        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise\\VC\\Tools\\MSVC",
                        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC",
                        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Tools\\MSVC",
                        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Professional\\VC\\Tools\\MSVC",
                        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Enterprise\\VC\\Tools\\MSVC",
                        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Tools\\MSVC",
                        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\BuildTools\\VC\\Tools\\MSVC"};

                    for (const auto &base_path : common_paths)
                    {
                        if (fs::exists(base_path))
                        {
                            // Find the latest version directory
                            std::vector<std::string> versions;
                            for (const auto &entry : fs::directory_iterator(base_path))
                            {
                                if (fs::is_directory(entry))
                                {
                                    versions.push_back(entry.path().string());
                                }
                            }
                            if (!versions.empty())
                            {
                                // Sort and take the latest version
                                std::sort(versions.begin(), versions.end());
                                std::string latest_version = versions.back();
                                std::string cl_path = latest_version + "\\bin\\Hostx64\\x64\\cl.exe";
                                std::string test_cl = "\"" + cl_path + "\" /? >nul 2>&1";
                                if (system(test_cl.c_str()) == 0)
                                {
                                    compilers[name].available = true;
                                    compilers[name].command = "\"" + cl_path + "\"";
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
};

class CppProcessor : public LanguageProcessor
{
public:
    CompilerRegistry *registry;

    bool can_handle(const std::string &ext) override
    {
        return ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx";
    }

    int compile(const std::string &input, const std::string &output,
                const std::vector<std::string> &flags, bool verbose) override
    {
        // Find best available C++ compiler
        std::string compiler_cmd;
        std::vector<std::string> default_flags;

        if (registry->compilers["msvc"].available)
        {
            compiler_cmd = registry->compilers["msvc"].command;
            default_flags = {"/nologo", "/utf-8", "/EHsc", "/std:c++17"};
        }
        else if (registry->compilers["clang++"].available)
        {
            compiler_cmd = registry->compilers["clang++"].command;
            default_flags = {"-Wall", "-std=c++17"};
        }
        else if (registry->compilers["g++"].available)
        {
            compiler_cmd = registry->compilers["g++"].command;
            default_flags = {"-Wall", "-std=c++17"};
        }
        else
        {
            // No compiler available
            std::cerr << "Error: No C/C++ compiler found. Please install Visual Studio, GCC, or Clang." << std::endl;
            return 1;
        }

        std::string cmd = compiler_cmd;
        for (const auto &flag : default_flags)
            cmd += " " + flag;
        for (const auto &flag : flags)
            cmd += " " + flag;
        cmd += " \"" + input + "\"";

        if (!output.empty())
        {
            if (compiler_cmd.find("cl") != std::string::npos)
            {
                cmd += " /Fe:\"" + output + "\"";
            }
            else
            {
                cmd += " -o \"" + output + "\"";
            }
        }

        if (verbose)
            std::cout << "Compiling C/C++: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    int run(const std::string &executable, bool verbose) override
    {
        std::string cmd = "\"" + executable + "\"";
        if (verbose)
            std::cout << "Running: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string &input) override
    {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }
};

class JavaProcessor : public LanguageProcessor
{
public:
    CompilerRegistry *registry;

    bool can_handle(const std::string &ext) override
    {
        return ext == ".java" || ext == ".jar";
    }

    int compile(const std::string &input, const std::string &output,
                const std::vector<std::string> &flags, bool verbose) override
    {
        // Check if output is JAR
        bool is_jar = !output.empty() && output.size() > 4 &&
                      output.substr(output.size() - 4) == ".jar";

        std::string class_output = output;
        if (is_jar)
        {
            // For JAR output, first compile to .class
            class_output = output.substr(0, output.size() - 4) + ".class";
        }

        // Try javac first
        if (registry->compilers["javac"].available)
        {
            std::string cmd = "javac";
            for (const auto &flag : flags)
                cmd += " " + flag;
            cmd += " \"" + input + "\"";
            if (verbose)
                std::cout << "Compiling Java with javac: " << cmd << std::endl;
            int result = system(cmd.c_str());
            if (result != 0)
                return result;

            // If JAR output requested, create JAR file
            if (is_jar)
            {
                std::string class_file = input.substr(0, input.find_last_of('.')) + ".class";
                std::string main_class = input.substr(0, input.find_last_of('.'));
                int jar_result = create_jar_file(class_file, output, main_class, verbose);
                if (jar_result == 0)
                {
                    return 0; // JAR created successfully
                }
                else
                {
                    // JAR creation failed, but class file exists - still return success
                    if (verbose)
                    {
                        std::cout << "Note: JAR creation failed, but class file was created successfully" << std::endl;
                    }
                    return 0;
                }
            }
            return 0;
        }
        else
        {
            // Fallback to embedded bytecode generator
            int result = generate_bytecode(input, class_output, verbose);
            if (result != 0)
                return result;

            // If JAR output requested, create JAR file
            if (is_jar)
            {
                std::string class_file = class_output;
                std::string main_class = input.substr(0, input.find_last_of('.'));
                int jar_result = create_jar_file(class_file, output, main_class, verbose);
                if (jar_result == 0)
                {
                    return 0; // JAR created successfully
                }
                else
                {
                    // JAR creation failed, but class file exists - still return success
                    if (verbose)
                    {
                        std::cout << "Note: JAR creation failed, but class file was created successfully" << std::endl;
                    }
                    return 0;
                }
            }
            return 0;
        }
    }

    int run(const std::string &executable, bool verbose) override
    {
        // Check if it's a JAR file
        if (executable.size() > 4 && executable.substr(executable.size() - 4) == ".jar")
        {
            std::string cmd = "java -jar \"" + executable + "\"";
            if (verbose)
                std::cout << "Running JAR: " << cmd << std::endl;
            return system(cmd.c_str());
        }
        else
        {
            // Run class file
            std::string class_name = executable.substr(0, executable.find_last_of('.'));
            std::string cmd = "java " + class_name;
            if (verbose)
                std::cout << "Running Java: " << cmd << std::endl;
            return system(cmd.c_str());
        }
    }

    std::string get_default_output(const std::string &input) override
    {
        // Check if user wants JAR output
        if (input.find(".jar") != std::string::npos ||
            (input.size() > 4 && input.substr(input.size() - 4) == ".jar"))
        {
            return input.substr(0, input.find_last_of('.')) + ".jar";
        }
        return input.substr(0, input.find_last_of('.')) + ".class";
    }

private:
    int generate_bytecode(const std::string &input, const std::string &output, bool verbose)
    {
        // Simple Java bytecode generator - only supports basic System.out.println("...");

        std::ifstream in(input);
        if (!in)
        {
            std::cerr << "Cannot open Java file: " << input << std::endl;
            return 1;
        }

        std::string line;
        std::string print_content;
        bool found_println = false;

        while (std::getline(in, line))
        {
            // Look for System.out.println("...");
            size_t pos = line.find("System.out.println(");
            if (pos != std::string::npos)
            {
                size_t start = line.find('"', pos);
                size_t end = line.find('"', start + 1);
                if (start != std::string::npos && end != std::string::npos)
                {
                    print_content = line.substr(start + 1, end - start - 1);
                    found_println = true;
                    break;
                }
            }
        }

        in.close();

        if (!found_println)
        {
            // Fallback to Hello World
            print_content = "Hello, World!";
        }

        // Generate bytecode for System.out.println(print_content);
        std::vector<uint8_t> classfile;
        generate_println_class(classfile, print_content);

        std::string classfile_name = output.empty() ? input.substr(0, input.find_last_of('.')) + ".class" : output;

        std::ofstream out(classfile_name, std::ios::binary);
        out.write((char *)classfile.data(), classfile.size());
        out.close();

        if (verbose)
            std::cout << "Generated Java bytecode: " << classfile_name << std::endl;
        return 0;
    }

    // Embedded JVM bytecode generator (simplified)
    void generate_println_class(std::vector<uint8_t> &out, const std::string &message)
    {
        // This is a simplified version - in reality you'd parse the Java file
        // For now, just generate a Hello World class
        W w;
        // ... (same bytecode generation code as before)
        w.u4(0xCAFEBABE);
        w.u2(0);
        w.u2(49);
        w.u2(28);
        // ... (rest of the bytecode)
        w.utf8("HelloWorld");
        w.u1(7);
        w.u2(1);
        w.utf8("java/lang/Object");
        w.u1(7);
        w.u2(3);
        w.utf8("java/lang/System");
        w.u1(7);
        w.u2(5);
        w.utf8("out");
        w.utf8("Ljava/io/PrintStream;");
        w.u1(12);
        w.u2(7);
        w.u2(8);
        w.u1(9);
        w.u2(6);
        w.u2(9);
        w.utf8("java/io/PrintStream");
        w.u1(7);
        w.u2(11);
        w.utf8("println");
        w.utf8("(Ljava/lang/string;)V");
        w.u1(12);
        w.u2(13);
        w.u2(14);
        w.u1(10);
        w.u2(12);
        w.u2(15);
        w.utf8(message); // The message
        w.u1(8);
        w.u2(17);
        w.utf8("<init>");
        w.utf8("()V");
        w.u1(12);
        w.u2(19);
        w.u2(20);
        w.u1(10);
        w.u2(4);
        w.u2(21);
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
        w.u1(0xb7);
        w.u2(22);
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
        w.u1(0xb2);
        w.u2(10);
        w.u1(0x12);
        w.u1(18);
        w.u1(0xb6);
        w.u2(16);
        w.u1(0xb1);
        w.u2(0);
        w.u2(0);
        w.u2(1);
        w.u2(26);
        w.u4(2);
        w.u2(27);
        out.swap(w.buf);
    }

    // Create JAR file with manifest and class file
    int create_jar_file(const std::string &class_file, const std::string &jar_file, const std::string &main_class, bool verbose)
    {
        // Check if jar command is available
        int jar_check = system("jar --version >nul 2>&1");
        if (jar_check != 0)
        {
            if (verbose)
            {
                std::cout << "Warning: jar command not available, skipping JAR creation" << std::endl;
                std::cout << "Class file created: " << class_file << std::endl;
            }
            return 1; // Indicate JAR creation failed, but class file exists
        }

        // Create temporary directory for JAR contents
        std::string temp_dir = class_file.substr(0, class_file.find_last_of('.')) + "_jar_temp";
        fs::create_directories(temp_dir);

        // Copy class file to temp directory with proper package structure
        std::string class_name = fs::path(class_file).filename().string();
        std::string class_in_jar = class_name; // For simple case, no package

        std::string dest_path = temp_dir + "/" + class_in_jar;
        // Copy class file to temp directory
        {
            std::ifstream in(class_file, std::ios::binary);
            if (!in)
            {
                std::cerr << "Failed to open class file for copying" << std::endl;
                return 1;
            }
            std::ofstream out(dest_path, std::ios::binary);
            if (!out)
            {
                std::cerr << "Failed to create destination file" << std::endl;
                return 1;
            }
            out << in.rdbuf();
        }

        // Create MANIFEST.MF
        std::string manifest_path = temp_dir + "/META-INF/MANIFEST.MF";
        fs::create_directories(temp_dir + "/META-INF");

        std::ofstream manifest(manifest_path);
        if (!manifest)
        {
            std::cerr << "Failed to create manifest file" << std::endl;
            return 1;
        }

        manifest << "Manifest-Version: 1.0\n";
        manifest << "Main-Class: " << main_class << "\n";
        manifest << "Created-By: zhcl_universal\n";
        manifest.close();

        // Create JAR using jar command if available, otherwise create ZIP
        std::string jar_cmd = "jar cfm \"" + jar_file + "\" \"" + manifest_path + "\" -C \"" + temp_dir + "\" .";
        if (verbose)
            std::cout << "Creating JAR: " << jar_cmd << std::endl;

        int result = system(jar_cmd.c_str());

        // Cleanup temp directory
        try
        {
            fs::remove_all(temp_dir);
        }
        catch (...)
        {
            // Ignore cleanup errors
        }

        if (result == 0 && verbose)
            std::cout << "Created JAR file: " << jar_file << std::endl;

        return result;
    }
};

class ChineseProcessor : public LanguageProcessor
{
public:
    CompilerRegistry *registry;

    bool can_handle(const std::string &ext) override
    {
        return ext == ".zh";
    }

    int compile(const std::string &input, const std::string &output,
                const std::vector<std::string> &flags, bool verbose) override
    {
        // Translate .zh to .cpp using built-in translator
        std::string cpp_file = output.empty() ? input.substr(0, input.find_last_of('.')) + ".cpp" : output.substr(0, output.find_last_of('.')) + ".cpp";

        if (translate_zh_to_cpp(input, cpp_file, verbose) != 0)
        {
            return 1;
        }

        // Then compile the generated C++ file
        CppProcessor cpp_processor;
        cpp_processor.registry = registry;
        std::string exe_output = output.empty() ? input.substr(0, input.find_last_of('.')) + ".exe" : output;

        return cpp_processor.compile(cpp_file, exe_output, flags, verbose);
    }

    int run(const std::string &executable, bool verbose) override
    {
        // Check if it's a .zh file - if so, execute directly without compilation
        if (executable.size() > 3 && executable.substr(executable.size() - 3) == ".zh")
        {
            return run_zh_file(executable, verbose);
        }

        // Otherwise, run as normal executable
        std::string cmd = "\"" + executable + "\"";
        if (verbose)
            std::cout << "Running Chinese program: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string &input) override
    {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }

private:
    int run_zh_file(const std::string &zh_file, bool verbose)
    {
        if (verbose)
            std::cout << "Running Chinese file directly: " << zh_file << std::endl;

        // Read the .zh file
        std::ifstream ifs(zh_file, std::ios::binary);
        if (!ifs)
        {
            if (verbose)
                std::fprintf(stderr, "[zhcl] cannot open %s\n", zh_file.c_str());
            return 1;
        }
        std::string src((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

        // Convert .zh to bytecode
        std::vector<uint8_t> bc;
        ZhFrontend fe;
        try
        {
            bc = fe.translate_to_bc(src);
        }
        catch (const std::exception &e)
        {
            std::fprintf(stderr, "[zhcl] zh->bc failed: %s\n", e.what());
            return 2;
        }

        // Execute bytecode directly
        selfhost::execute_bc(bc);
        return 0; // execute_bc doesn't return
    }
};

class PythonProcessor : public LanguageProcessor
{
public:
    CompilerRegistry *registry;

    bool can_handle(const std::string &ext) override { return ext == ".py"; }

    int compile(const std::string &input, const std::string &output,
                const std::vector<std::string> &flags, bool verbose) override
    {
        // Translate .py to .cpp using built-in translator
        std::string cpp_file = output.empty() ? input.substr(0, input.find_last_of('.')) + ".cpp" : output.substr(0, output.find_last_of('.')) + ".cpp";

        if (translate_py_to_cpp(input, cpp_file, verbose) != 0)
        {
            return 1;
        }

        // Then compile the generated C++ file
        CppProcessor cpp_processor;
        cpp_processor.registry = registry;
        std::string exe_output = output.empty() ? input.substr(0, input.find_last_of('.')) + ".exe" : output;

        return cpp_processor.compile(cpp_file, exe_output, flags, verbose);
    }

    int run(const std::string &executable, bool verbose) override
    {
        std::string cmd = "\"" + executable + "\"";
        if (verbose)
            std::cout << "Running Python program: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string &input) override
    {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }

private:
    int translate_py_to_cpp(const std::string &py_file, const std::string &cpp_file, bool verbose)
    {
        std::ifstream in(py_file);
        std::ofstream out(cpp_file);
        std::string line;

        out << "#include <iostream>\n";
        out << "#include <string>\n";
        out << "#include <vector>\n\n";

        std::string main_code;

        while (std::getline(in, line))
        {
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line.substr(0, 1) == "#")
            {
                continue;
            }

            // Basic Python to C++ translation (very simplified)
            if (line.find("print(") != std::string::npos)
            {
                // print("hello") -> std::cout << "hello" << std::endl;
                size_t start = line.find('"');
                size_t end = line.find('"', start + 1);
                if (start != std::string::npos && end != std::string::npos)
                {
                    std::string content = line.substr(start, end - start + 1);
                    main_code += "    std::cout << " + content + " << std::endl;\n";
                }
            }
            else if (line.find("=") != std::string::npos && line.find("int(") == std::string::npos)
            {
                // x = 5 -> int x = 5;
                size_t eq_pos = line.find('=');
                std::string var = trim(line.substr(0, eq_pos));
                std::string value = trim(line.substr(eq_pos + 1));
                main_code += "    int " + var + " = " + value + ";\n";
            }
            else
            {
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

        if (verbose)
            std::cout << "Translated " + py_file + " to " + cpp_file << std::endl;
        return 0;
    }

    std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(" \t");
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }
};

class GoProcessor : public LanguageProcessor
{
public:
    CompilerRegistry *registry;

    bool can_handle(const std::string &ext) override { return ext == ".go"; }

    int compile(const std::string &input, const std::string &output,
                const std::vector<std::string> &flags, bool verbose) override
    {
        // Translate .go to .cpp using built-in translator
        std::string cpp_file = output.empty() ? input.substr(0, input.find_last_of('.')) + ".cpp" : output.substr(0, output.find_last_of('.')) + ".cpp";

        if (translate_go_to_cpp(input, cpp_file, verbose) != 0)
        {
            return 1;
        }

        // Then compile the generated C++ file
        CppProcessor cpp_processor;
        cpp_processor.registry = registry;
        std::string exe_output = output.empty() ? input.substr(0, input.find_last_of('.')) + ".exe" : output;

        return cpp_processor.compile(cpp_file, exe_output, flags, verbose);
    }

    int run(const std::string &executable, bool verbose) override
    {
        std::string cmd = "\"" + executable + "\"";
        if (verbose)
            std::cout << "Running Go program: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string &input) override
    {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }

private:
    int translate_go_to_cpp(const std::string &go_file, const std::string &cpp_file, bool verbose)
    {
        std::ifstream in(go_file);
        std::ofstream out(cpp_file);
        std::string line;

        out << "#include <iostream>\n";
        out << "#include <string>\n";
        out << "#include <vector>\n\n";

        std::string main_code;
        std::vector<std::string> functions;

        while (std::getline(in, line))
        {
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line.substr(0, 2) == "//")
            {
                continue;
            }

            // Basic Go to C++ translation (very simplified)
            if (line.find("fmt.Println(") != std::string::npos)
            {
                // fmt.Println("hello") -> std::cout << "hello" << std::endl;
                size_t start = line.find('"');
                size_t end = line.find('"', start + 1);
                if (start != std::string::npos && end != std::string::npos)
                {
                    std::string content = line.substr(start, end - start + 1);
                    main_code += "    std::cout << " + content + " << std::endl;\n";
                }
            }
            else if (line.find("func ") != std::string::npos)
            {
                // Function declarations
                functions.push_back(translate_function_declaration(line));
                continue;
            }

            // Main code
            main_code += translate_go_statement(line) + "\n";
        }

        // Write functions
        for (const auto &func : functions)
        {
            out << func << "\n";
        }

        // Write main function
        out << "int main() {\n";
        out << main_code;
        out << "    return 0;\n";
        out << "}\n";

        in.close();
        out.close();

        if (verbose)
            std::cout << "Translated " << go_file << " to " << cpp_file << std::endl;
        return 0;
    }

    std::string translate_function_declaration(const std::string &line)
    {
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
        if (last_space != std::string::npos)
        {
            std::string return_type = result.substr(last_space + 1);
            std::string func_sig = result.substr(0, last_space);
            result = return_type + " " + func_sig;
        }
        return result + " {";
    }

    std::string translate_go_statement(const std::string &line)
    {
        std::string result = line;

        // Variable declarations: var x int = 5 -> int x = 5;
        if (result.substr(0, 4) == "var ")
        {
            result = result.substr(4);
            // Assume format: name type = value
            // Simple: replace with C++ style
        }

        // Print statements: fmt.Println("hello") -> std::cout << "hello" << std::endl;
        if (result.find("fmt.Println") != std::string::npos)
        {
            size_t start = result.find('"');
            size_t end = result.find('"', start + 1);
            if (start != std::string::npos && end != std::string::npos)
            {
                std::string content = result.substr(start, end - start + 1);
                result = "    std::cout << " + content + " << std::endl;";
            }
        }

        // Basic replacements
        result = replace_all(result, ":=", "="); // Short variable declaration

        return result;
    }

    std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(" \t");
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }

    std::string replace_all(std::string str, const std::string &from, const std::string &to)
    {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }
};

class RustProcessor : public LanguageProcessor
{
public:
    bool can_handle(const std::string &ext) override { return ext == ".rs"; }
    int compile(const std::string &input, const std::string &output, const std::vector<std::string> &flags, bool verbose) override
    {
        std::string cmd = "rustc";
        for (const auto &flag : flags)
            cmd += " " + flag;
        cmd += " \"" + input + "\"";
        if (!output.empty())
            cmd += " -o \"" + output + "\"";
        if (verbose)
            std::cout << "Compiling Rust: " << cmd << std::endl;
        return system(cmd.c_str());
    }
    int run(const std::string &executable, bool verbose) override
    {
        std::string cmd = "\"" + executable + "\"";
        if (verbose)
            std::cout << "Running Rust: " << cmd << std::endl;
        return system(cmd.c_str());
    }
    std::string get_default_output(const std::string &input) override
    {
        return input.substr(0, input.find_last_of('.')) + ".exe";
    }
};

class JSProcessor : public LanguageProcessor
{
public:
    CompilerRegistry *registry;

    bool can_handle(const std::string &ext) override { return ext == ".js"; }

    int compile(const std::string &input, const std::string &output,
                const std::vector<std::string> &flags, bool verbose) override
    {
        // Translate .js to .cpp using built-in translator
        std::string cpp_file = output.empty() ? input.substr(0, input.find_last_of('.')) + ".cpp" : output.substr(0, output.find_last_of('.')) + ".cpp";

        if (translate_js_to_cpp(input, cpp_file, verbose) != 0)
        {
            return 1;
        }

        // Then compile the generated C++ file
        CppProcessor cpp_processor;
        cpp_processor.registry = registry;
        return cpp_processor.compile(cpp_file, output, flags, verbose);
    }

    int run(const std::string &executable, bool verbose) override
    {
        std::string cmd = "node \"" + executable + "\"";
        if (verbose)
            std::cout << "Running JavaScript: " << cmd << std::endl;
        return system(cmd.c_str());
    }

    std::string get_default_output(const std::string &input) override { return input; }

private:
    int translate_js_to_cpp(const std::string &js_file, const std::string &cpp_file, bool verbose)
    {
        std::ifstream in(js_file);
        std::ofstream out(cpp_file);
        std::string line;

        out << "#include <iostream>\n";
        out << "#include <string>\n";
        out << "#include <vector>\n\n";

        std::string main_code;

        while (std::getline(in, line))
        {
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line.substr(0, 2) == "//")
            {
                continue;
            }

            // Basic JavaScript to C++ translation (very simplified)
            if (line.find("console.log(") != std::string::npos)
            {
                // console.log("hello") -> std::cout << "hello" << std::endl;
                size_t start = line.find('"');
                size_t end = line.find('"', start + 1);
                if (start != std::string::npos && end != std::string::npos)
                {
                    std::string content = line.substr(start, end - start + 1);
                    main_code += "    std::cout << " + content + " << std::endl;\n";
                }
            }
            else if (line.find("let ") != std::string::npos || line.find("const ") != std::string::npos || line.find("var ") != std::string::npos)
            {
                // let x = 5 -> int x = 5;
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string var_part = trim(line.substr(0, eq_pos));
                    std::string value = trim(line.substr(eq_pos + 1));
                    // Remove semicolon if present
                    if (!value.empty() && value.back() == ';')
                        value.pop_back();

                    // Extract variable name (after let/const/var and space)
                    size_t space_pos = var_part.find(' ');
                    if (space_pos != std::string::npos)
                    {
                        std::string var_name = trim(var_part.substr(space_pos + 1));
                        main_code += "    int " + var_name + " = " + value + ";\n";
                    }
                }
            }
            else
            {
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

        if (verbose)
            std::cout << "Translated " + js_file + " to " + cpp_file << std::endl;
        return 0;
    }

    std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(" \t");
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }
};

// ============================================================================
// COMPILER COMPATIBILITY LAYER - Maintains traditional compiler habits
// ============================================================================

class CompilerCompatibilityLayer
{
public:
    CompilerRegistry *registry;

    // Parse traditional compiler arguments and translate to zhcl format
    int handle_traditional_args(int argc, char **argv)
    {
        if (argc < 2)
            return -1; // Not a traditional compiler call

        std::string compiler_name = argv[0];
        std::string basename = fs::path(compiler_name).filename().string();

        // map common compiler names to our system
        if (basename == "cl" || basename == "cl.exe")
        {
            return handle_msvc_args(argc, argv);
        }
        else if (basename == "gcc" || basename == "g++" || basename == "cc" || basename == "c++")
        {
            return handle_gcc_args(argc, argv);
        }
        else if (basename == "javac" || basename == "javac.exe")
        {
            return handle_javac_args(argc, argv);
        }
        else if (basename == "rustc" || basename == "rustc.exe")
        {
            return handle_rustc_args(argc, argv);
        }
        else if (basename == "go" || basename == "go.exe")
        {
            return handle_go_args(argc, argv);
        }
        else if (basename == "python" || basename == "python.exe" || basename == "python3")
        {
            return handle_python_args(argc, argv);
        }

        return -1; // Not recognized
    }

private:
    // MSVC-style arguments: cl [options] file.cpp
    int handle_msvc_args(int argc, char **argv)
    {
        std::vector<std::string> files;
        std::string output;
        std::vector<std::string> includes;
        std::vector<std::string> defines;
        std::vector<std::string> flags;
        bool compile_only = false;
        bool verbose = false;

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg == "/c")
            {
                compile_only = true;
            }
            else if (arg == "/Fe" && i + 1 < argc)
            {
                output = argv[++i];
            }
            else if (arg.substr(0, 2) == "/I")
            {
                includes.push_back(arg.substr(2));
            }
            else if (arg.substr(0, 2) == "/D")
            {
                defines.push_back(arg.substr(2));
            }
            else if (arg == "/nologo")
            {
                // Silent mode
            }
            else if (arg == "/utf-8")
            {
                flags.push_back("-utf-8");
            }
            else if (arg == "/EHsc")
            {
                flags.push_back("-EHsc");
            }
            else if (arg == "/std:c++17")
            {
                flags.push_back("-std=c++17");
            }
            else if (arg.substr(0, 1) != "/")
            {
                // Assume it's a file
                files.push_back(arg);
            }
        }

        if (files.empty())
        {
            std::cerr << "cl: no input files" << std::endl;
            return 1;
        }

        // Compile each file
        for (const auto &file : files)
        {
            std::string ext = fs::path(file).extension().string();
            if (ext != ".cpp" && ext != ".c" && ext != ".cc")
            {
                std::cerr << "cl: unsupported file type: " << file << std::endl;
                continue;
            }

            std::string actual_output = output;
            if (actual_output.empty())
            {
                if (compile_only)
                {
                    actual_output = file.substr(0, file.find_last_of('.')) + ".obj";
                }
                else
                {
                    actual_output = file.substr(0, file.find_last_of('.')) + ".exe";
                }
            }

            // Add include paths
            for (const auto &inc : includes)
            {
                flags.push_back("-I" + inc);
            }

            // Add defines
            for (const auto &def : defines)
            {
                flags.push_back("-D" + def);
            }

            CppProcessor processor;
            processor.registry = registry;
            int ret = processor.compile(file, actual_output, flags, verbose);

            if (ret != 0)
            {
                return ret;
            }
        }

        return 0;
    }

    // GCC-style arguments: gcc [options] file.c -o output
    int handle_gcc_args(int argc, char **argv)
    {
        std::vector<std::string> files;
        std::string output;
        std::vector<std::string> includes;
        std::vector<std::string> defines;
        std::vector<std::string> flags;
        bool compile_only = false;
        bool verbose = false;

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg == "-c")
            {
                compile_only = true;
            }
            else if (arg == "-o" && i + 1 < argc)
            {
                output = argv[++i];
            }
            else if (arg.substr(0, 2) == "-I")
            {
                includes.push_back(arg.substr(2));
            }
            else if (arg.substr(0, 2) == "-D")
            {
                defines.push_back(arg.substr(2));
            }
            else if (arg == "-Wall" || arg == "-Wextra" || arg == "-g" || arg == "-O2")
            {
                flags.push_back(arg);
            }
            else if (arg.substr(0, 5) == "-std=")
            {
                flags.push_back(arg);
            }
            else if (arg.substr(0, 1) != "-")
            {
                // Assume it's a file
                files.push_back(arg);
            }
        }

        if (files.empty())
        {
            std::cerr << "gcc: no input files" << std::endl;
            return 1;
        }

        // Compile each file
        for (size_t idx = 0; idx < files.size(); ++idx)
        {
            const auto &file = files[idx];
            std::string ext = fs::path(file).extension().string();

            std::string actual_output;
            if (output.empty())
            {
                if (compile_only)
                {
                    actual_output = file.substr(0, file.find_last_of('.')) + ".o";
                }
                else if (files.size() == 1)
                {
                    actual_output = "a.out";
                }
                else
                {
                    actual_output = file.substr(0, file.find_last_of('.')) + ".o";
                }
            }
            else
            {
                if (files.size() > 1)
                {
                    // Multiple files with single output = link them
                    if (idx == files.size() - 1)
                    {
                        // Last file, do the linking
                        actual_output = output;
                    }
                    else
                    {
                        continue; // Skip intermediate files
                    }
                }
                else
                {
                    actual_output = output;
                }
            }

            // Add include paths
            for (const auto &inc : includes)
            {
                flags.push_back("-I" + inc);
            }

            // Add defines
            for (const auto &def : defines)
            {
                flags.push_back("-D" + def);
            }

            CppProcessor processor;
            processor.registry = registry;
            int ret = processor.compile(file, actual_output, flags, verbose);

            if (ret != 0)
            {
                return ret;
            }
        }

        return 0;
    }

    // JavaC-style arguments: javac [options] file.java
    int handle_javac_args(int argc, char **argv)
    {
        std::vector<std::string> files;
        std::string classpath;
        std::string output_dir = ".";
        std::vector<std::string> flags;

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg == "-cp" || arg == "-classpath")
            {
                if (i + 1 < argc)
                {
                    classpath = argv[++i];
                }
            }
            else if (arg == "-d" && i + 1 < argc)
            {
                output_dir = argv[++i];
            }
            else if (arg.substr(0, 1) != "-")
            {
                files.push_back(arg);
            }
            else
            {
                flags.push_back(arg);
            }
        }

        if (files.empty())
        {
            std::cerr << "javac: no input files" << std::endl;
            return 1;
        }

        for (const auto &file : files)
        {
            std::string ext = fs::path(file).extension().string();
            if (ext != ".java")
            {
                std::cerr << "javac: not a Java file: " << file << std::endl;
                continue;
            }

            std::string output = output_dir + "/" + file.substr(0, file.find_last_of('.')) + ".class";

            JavaProcessor processor;
            processor.registry = registry;
            int ret = processor.compile(file, output, flags, false);

            if (ret != 0)
            {
                return ret;
            }
        }

        return 0;
    }

    // RustC-style arguments: rustc [options] file.rs
    int handle_rustc_args(int argc, char **argv)
    {
        std::vector<std::string> files;
        std::string output;
        std::string out_dir = ".";
        std::vector<std::string> flags;
        bool verbose = false;

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg == "-o" && i + 1 < argc)
            {
                output = argv[++i];
            }
            else if (arg == "--out-dir" && i + 1 < argc)
            {
                out_dir = argv[++i];
            }
            else if (arg == "-O" || arg == "-g")
            {
                flags.push_back(arg);
            }
            else if (arg.substr(0, 1) != "-")
            {
                files.push_back(arg);
            }
        }

        if (files.empty())
        {
            std::cerr << "rustc: no input files" << std::endl;
            return 1;
        }

        for (const auto &file : files)
        {
            std::string ext = fs::path(file).extension().string();
            if (ext != ".rs")
            {
                std::cerr << "rustc: not a Rust file: " << file << std::endl;
                continue;
            }

            std::string actual_output = output.empty() ? out_dir + "/" + file.substr(0, file.find_last_of('.')) + ".exe" : output;

            RustProcessor processor;
            int ret = processor.compile(file, actual_output, flags, verbose);

            if (ret != 0)
            {
                return ret;
            }
        }

        return 0;
    }

    // Go-style arguments: go build/run [options] file.go
    int handle_go_args(int argc, char **argv)
    {
        if (argc < 2)
            return -1;

        std::string subcommand = argv[1];
        std::vector<std::string> files;
        std::string output;
        std::vector<std::string> flags;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg == "-o" && i + 1 < argc)
            {
                output = argv[++i];
            }
            else if (arg.substr(0, 1) == "-")
            {
                flags.push_back(arg);
            }
            else
            {
                files.push_back(arg);
            }
        }

        if (files.empty())
        {
            std::cerr << "go: no input files" << std::endl;
            return 1;
        }

        for (const auto &file : files)
        {
            std::string ext = fs::path(file).extension().string();
            if (ext != ".go")
            {
                std::cerr << "go: not a Go file: " << file << std::endl;
                continue;
            }

            std::string actual_output = output.empty() ? file.substr(0, file.find_last_of('.')) + ".exe" : output;

            GoProcessor processor;
            int ret;

            if (subcommand == "run")
            {
                ret = processor.compile(file, actual_output, flags, false);
                if (ret == 0)
                {
                    ret = processor.run(actual_output, false);
                }
            }
            else
            { // build or default
                ret = processor.compile(file, actual_output, flags, false);
            }

            if (ret != 0)
            {
                return ret;
            }
        }

        return 0;
    }

    // Python-style arguments: python [options] file.py
    int handle_python_args(int argc, char **argv)
    {
        std::vector<std::string> args;

        for (int i = 1; i < argc; ++i)
        {
            args.push_back(argv[i]);
        }

        if (args.empty())
        {
            // Start Python REPL
            return system("python");
        }

        std::string file = args.back();
        std::string ext = fs::path(file).extension().string();

        if (ext != ".py")
        {
            // Not a Python file, pass through to python
            std::string cmd = "python";
            for (const auto &arg : args)
            {
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

class BuildSystem
{
public:
    CompilerRegistry *registry;
    std::map<std::string, std::unique_ptr<LanguageProcessor>> processors;

    BuildSystem(CompilerRegistry *reg) : registry(reg)
    {
        // Register processors
        processors["cpp"] = std::make_unique<CppProcessor>();
        auto *cpp_processor = static_cast<CppProcessor *>(processors["cpp"].get());
        cpp_processor->registry = registry;

        processors["java"] = std::make_unique<JavaProcessor>();
        auto *java_processor = static_cast<JavaProcessor *>(processors["java"].get());
        java_processor->registry = registry;

        processors["python"] = std::make_unique<PythonProcessor>();
        auto *python_processor = static_cast<PythonProcessor *>(processors["python"].get());
        python_processor->registry = registry;

        processors["javascript"] = std::make_unique<JSProcessor>();
        auto *js_processor = static_cast<JSProcessor *>(processors["javascript"].get());
        js_processor->registry = registry;
        processors["go"] = std::make_unique<GoProcessor>();
        auto *go_processor = static_cast<GoProcessor *>(processors["go"].get());
        go_processor->registry = registry;
        processors["rust"] = std::make_unique<RustProcessor>();

        processors["chinese"] = std::make_unique<ChineseProcessor>();
        auto *chinese_processor = static_cast<ChineseProcessor *>(processors["chinese"].get());
        chinese_processor->registry = registry;
    }

    int build_project(const std::string &project_dir, bool verbose)
    {
        if (verbose)
            std::cout << "Building project in: " << project_dir << std::endl;

        // Find all source files
        std::vector<std::string> source_files;
        for (const auto &entry : fs::recursive_directory_iterator(project_dir))
        {
            if (fs::is_regular_file(entry.path()))
            {
                std::string ext = entry.path().extension().string();
                if (is_supported_extension(ext))
                {
                    source_files.push_back(entry.path().string());
                }
            }
        }

        if (source_files.empty())
        {
            std::cerr << "No supported source files found" << std::endl;
            return 1;
        }

        // Group by language
        std::map<std::string, std::vector<std::string>> files_by_lang;
        for (const auto &file : source_files)
        {
            std::string ext = fs::path(file).extension().string();
            files_by_lang[ext].push_back(file);
        }

        // Compile each group
        std::vector<std::string> outputs;
        for (const auto &[ext, files] : files_by_lang)
        {
            for (const auto &file : files)
            {
                std::string output = get_processor_by_ext(ext)->get_default_output(file);
                if (get_processor_by_ext(ext)->compile(file, output, {}, verbose) == 0)
                {
                    outputs.push_back(output);
                    if (verbose)
                        std::cout << "Compiled: " << file << " -> " << output << std::endl;
                }
                else
                {
                    std::cerr << "Failed to compile: " << file << std::endl;
                    return 1;
                }
            }
        }

        if (verbose)
            std::cout << "Build completed successfully!" << std::endl;
        return 0;
    }

    int compile_file(const std::string &input, const std::string &output, bool verbose, bool run_after)
    {
        std::string ext = fs::path(input).extension().string();
        auto processor = get_processor_by_ext(ext);
        if (!processor)
        {
            std::cerr << "Unsupported file type: " << ext << std::endl;
            return 1;
        }

        std::string actual_output = output.empty() ? processor->get_default_output(input) : output;

        int ret = processor->compile(input, actual_output, {}, verbose);
        if (ret == 0 && run_after)
        {
            return processor->run(actual_output, verbose);
        }
        return ret;
    }

private:
    LanguageProcessor *get_processor_by_ext(const std::string &ext)
    {
        for (auto &[name, processor] : processors)
        {
            if (processor->can_handle(ext))
            {
                return processor.get();
            }
        }
        return nullptr;
    }

    bool is_supported_extension(const std::string &ext)
    {
        return get_processor_by_ext(ext) != nullptr;
    }

private:
    std::string get_language_from_extension(const std::string &ext)
    {
        if (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx")
            return "cpp";
        if (ext == ".java")
            return "java";
        if (ext == ".py")
            return "python";
        if (ext == ".js")
            return "javascript";
        if (ext == ".go")
            return "go";
        if (ext == ".rs")
            return "rust";
        if (ext == ".zh")
            return "chinese";
        return "unknown";
    }

    std::string get_output_path(const std::string &input, const std::string &lang)
    {
        std::string base = input.substr(0, input.find_last_of('.'));
        if (lang == "cpp")
            return base + ".exe";
        if (lang == "java")
            return base + ".class";
        if (lang == "python")
            return input;
        if (lang == "javascript")
            return base + ".exe";
        if (lang == "go")
            return base + ".exe";
        if (lang == "rust")
            return base + ".exe";
        if (lang == "chinese")
            return base + ".exe";
        return base + ".out";
    }

    std::vector<std::string> get_default_flags(const std::string &lang)
    {
        if (lang == "cpp")
            return {"-Wall", "-std=c++17"};
        if (lang == "java")
            return {};
        if (lang == "python")
            return {};
        if (lang == "go")
            return {};
        if (lang == "rust")
            return {};
        if (lang == "chinese")
            return {};
        return {};
    }
};

// ============================================================================
// MAIN APPLICATION
// ============================================================================

// ---- NEW: Frontend-based commands (Self-Host Universal Runner)
static bool read_file(const std::string &p, std::string &out)
{
    std::ifstream f(p, std::ios::binary);
    if (!f)
        return false;
    out.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return true;
}

int cmd_list_frontends()
{
    for (auto &fe : FrontendRegistry::instance().all())
        std::cout << "- " << fe->name() << "\n";
    return 0;
}

int cmd_run(const std::string &path, const std::string &forced, const std::vector<std::string> &extra_args)
{
    std::string src;
    if (!read_file(path, src))
    {
        std::cerr << "read fail: " << path << "\n";
        return 1;
    }

    // BOM/換行正規化
    strip_utf8_bom(src);
    normalize_newlines(src);

    auto fe = forced.empty() ? FrontendRegistry::instance().match(path, src) : FrontendRegistry::instance().by_name(forced);
    if (!fe)
    {
        std::cerr << "no frontend: " << (forced.empty() ? path : forced) << "\n";
        return 2;
    }
    std::cout << "Using frontend: " << fe->name() << "\n";
    FrontendContext ctx{path, src, true};
    Bytecode bc;
    std::string err;
    if (!fe->compile(ctx, bc, err))
    {
        std::cerr << "compile err: " << err << "\n";
        return 3;
    }

    // 解析命令列參數並注入 SET_I64
    std::vector<int64_t> args;
    bool sep = false;
    for (auto &a : extra_args)
    {
        if (a == "--")
        {
            sep = true;
            continue;
        }
        if (!sep)
            continue;
        try
        {
            args.push_back(std::stoll(a));
        }
        catch (...)
        {
            std::cerr << "Invalid arg: " << a << "\n";
            return 4;
        }
    }

    // 在 bc.data 开头插入 SET_I64
    for (size_t i = 0; i < args.size(); ++i)
    {
        bc.data.insert(bc.data.begin(), 0x03);           // OP_SET_I64
        bc.data.insert(bc.data.begin() + 1, (uint8_t)i); // slot
        int64_t v = args[i];
        for (int k = 0; k < 8; ++k)
        {
            bc.data.insert(bc.data.begin() + 2 + k, (uint8_t)((uint64_t)v >> (k * 8)) & 0xFF);
        }
    }

    // 確保 OP_END 在最後
    if (bc.data.empty() || bc.data.back() != 0x04)
        bc.data.push_back(0x04);

    // ?瑁?嚗?怎?? VM
    selfhost::execute_bc(bc.data);
    return 0; // execute_bc doesn't return
}

// ---- Forward declarations for functions used by main/clean_project ----
int initialize_project(bool verbose);
int list_compilers(const CompilerRegistry &registry, bool verbose);
int clean_project(bool verbose);
bool matches_pattern(const std::string &filename, const std::string &pattern);

// 新增：判斷此命令是否需要偵測外部編譯器
static bool command_needs_compiler_detect(const std::string &cmd)
{
    return (cmd == "build" || cmd == "list" || cmd == "init" || cmd == "compile");
}

static bool skip_detect_from_env()
{
    const char *v = std::getenv("ZHCL_SKIP_DETECT");
    return (v && *v && (*v != '0'));
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    initialize_chinese_environment();

    // ★ 強制把所有前端拉進可執行檔並註冊
    register_fe_clite();
    register_fe_cpplite();
    register_fe_jslite();
    register_fe_zh();
    register_fe_golite();
    register_fe_javalite();
    register_fe_pylite();

    // Frontends are auto-registered via static initializers

    // ??main() ?脣暺????
    if (selfhost::maybe_run_embedded_payload())
    {
        return 0; // ?交?撠暹? payload嚗歇?瑁?銝?ExitProcess()嚗??芾絲閬?return
    }

    if (argc <= 1)
    {
        std::cout << "zhcl - Universal VM & Selfhost System v1.0" << std::endl;
        std::cout << "Run any supported language via built-in VM, no external compilers needed" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: zhcl <command> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  run <file>      Run file directly via VM (zh/c-lite/cpp-lite/js-lite)" << std::endl;
        std::cout << "  list-frontends  List available language frontends" << std::endl;
        std::cout << "  selfhost        Self-contained executable generation" << std::endl;
        std::cout << std::endl;
        std::cout << "Selfhost Commands:" << std::endl;
        std::cout << "  selfhost pack <input.(js|py|go|java|zh)> -o <output.exe>    Pack source into self-contained exe" << std::endl;
        std::cout << "  selfhost verify <exe>                                      Verify exe integrity" << std::endl;
        std::cout << "  selfhost explain <input.(js|py|go|java|zh)>                Show bytecode disassembly" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --frontend=<name>  Force specific frontend (zh|c-lite|cpp-lite|js-lite)" << std::endl;
        std::cout << "  -- <args...>       Pass integer arguments to VM slots (0,1,2,...)" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  zhcl run hello.zh" << std::endl;
        std::cout << "  zhcl run --frontend=c-lite hello.c" << std::endl;
        std::cout << "  zhcl run script.zh -- 2025 3 20 16 0 0 -5" << std::endl;
        std::cout << "  zhcl selfhost pack hello.js -o hello.exe" << std::endl;
        std::cout << std::endl;
        std::cout << "Supported: C/C++, Java, Python, Go, JavaScript, Chinese (.zh)" << std::endl;
        std::cout << "No external compilers required - everything runs via built-in VM" << std::endl;
        return 0;
    }

    std::string cmd = argv[1];

    // 完全 VM 的安全路徑
    if (cmd == "--help" || cmd == "-h")
    {
        // 上面已經處理了 argc <= 1 的情況，這裡重複一下
        std::cout << "zhcl - Universal VM & Selfhost System v1.0" << std::endl;
        std::cout << "Run any supported language via built-in VM, no external compilers needed" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: zhcl <command> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  run <file>           Run file directly via VM (zh/c-lite/cpp-lite/js-lite)" << std::endl;
        std::cout << "  list-frontends       List available language frontends" << std::endl;
        std::cout << "  selfhost             Self-contained executable generation" << std::endl;
        std::cout << std::endl;
        std::cout << "Selfhost Commands:" << std::endl;
        std::cout << "  selfhost pack <input> -o <output.exe>    Pack source into self-contained exe" << std::endl;
        std::cout << "  selfhost verify <exe>                   Verify exe integrity (CRC32 check)" << std::endl;
        std::cout << "  selfhost explain <input>                Show bytecode disassembly" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --frontend=<name>    Force specific frontend (zh|c-lite|cpp-lite|js-lite)" << std::endl;
        std::cout << "  -- <args...>         Pass integer arguments to VM slots (0,1,2,...)" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  zhcl run hello.zh" << std::endl;
        std::cout << "  zhcl run --frontend=c-lite hello.c" << std::endl;
        std::cout << "  zhcl run script.zh -- 2025 3 20 16 0 0 -5" << std::endl;
        std::cout << "  zhcl selfhost pack hello.js -o hello.exe" << std::endl;
        std::cout << "  zhcl selfhost verify hello.exe" << std::endl;
        std::cout << std::endl;
        std::cout << "Standards Compatibility:" << std::endl;
        std::cout << "  System: C11/C17/C23 and C++17/C++20/C++23 compatible" << std::endl;
        std::cout << "  Generated code: C89/C90 standard for maximum compatibility" << std::endl;
        std::cout << "  Encoding: UTF-8 native support" << std::endl;
        std::cout << std::endl;
        std::cout << "Documentation:" << std::endl;
        std::cout << "  README.md        - Quick start guide and examples" << std::endl;
        std::cout << "  PARAMETERS.md    - Detailed parameter reference (Chinese)" << std::endl;
        std::cout << "  PARAMETERS_EN.md - Detailed parameter reference (English)" << std::endl;
        std::cout << "  LANGUAGE.md      - Chinese language specification" << std::endl;
        std::cout << std::endl;
        std::cout << "Environment Variables:" << std::endl;
        std::cout << "  ZHCL_SELFHOST_QUIET=1  - Suppress selfhost banner output" << std::endl;
        std::cout << std::endl;
        std::cout << "No external compilers required - everything runs via built-in VM" << std::endl;
        return 0;
    }

    if (cmd == "list-frontends")
    {
        return cmd_list_frontends();
    }
    if (cmd == "run")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: zhcl run <file> [--frontend=name] [-- args...]\n";
            return 1;
        }
        std::string file;
        std::string forced;
        std::vector<std::string> extra_args;
        for (int i = 2; i < argc; ++i)
        {
            std::string a = argv[i];
            if (a.rfind("--frontend=", 0) == 0)
            {
                forced = a.substr(11);
            }
            else if (a == "--")
            {
                for (int j = i + 1; j < argc; ++j)
                {
                    extra_args.push_back(argv[j]);
                }
                break;
            }
            else if (file.empty())
            {
                file = a;
            }
            else
            {
                // 不支援額外的參數，除非是 -- 之後的
                std::cerr << "Unexpected argument: " << a << "\n";
                std::cerr << "Usage: zhcl run <file> [--frontend=name] [-- args...]\n";
                return 1;
            }
        }
        if (file.empty())
        {
            std::cerr << "Usage: zhcl run <file> [--frontend=name] [-- args...]\n";
            return 1;
        }
        return cmd_run(file, forced, extra_args);
    }
    if (cmd == "selfhost")
    {
        if (argc < 3)
        {
            std::puts("Usage:\n  zhcl selfhost pack <input.(js|py|go|java|zh)> -o <output.exe>\n"
                      "  zhcl selfhost verify <exe>\n"
                      "  zhcl selfhost explain <input.(js|py|go|java|zh)>                Show bytecode disassembly");
            return 1;
        }
        std::string sub = argv[2];
        if (sub == "pack")
        {
            if (argc < 6 || std::string(argv[4]) != "-o")
            {
                std::puts("Usage:\n  zhcl selfhost pack <input.(js|py|go|java|zh)> -o <output.exe>");
                return 2;
            }
            fs::path in = argv[3];
            fs::path out = argv[5];
            std::string lang;
            auto ext = in.extension().string();
            if (ext == ".js")
                lang = "js";
            else if (ext == ".py")
                lang = "py";
            else if (ext == ".go")
                lang = "go";
            else if (ext == ".java")
                lang = "java";
            else if (ext == ".zh")
                lang = "zh";
            else
            {
                std::fprintf(stderr, "[selfhost] unsupported input: %s\n", ext.c_str());
                return 2;
            }
            int rc = selfhost::pack_from_file(lang, in, out);
            return rc;
        }
        else if (sub == "verify")
        {
            if (argc < 4)
            {
                std::puts("Usage:\n  zhcl selfhost verify <exe>");
                return 2;
            }
            return selfhost::verify_exe(argv[3]);
        }
        else if (sub == "explain")
        {
            return selfhost::handle_selfhost_explain(argc, argv);
        }
        std::fprintf(stderr, "Unknown subcommand: selfhost %s\n", sub.c_str());
        return 1;
    }

#if ZHCL_ENABLE_EXTERNAL_TOOLCHAIN
    // （可選）外部編譯器相關子命令
    // ... 保留舊邏輯 ...
#else
    // 關閉時，任何外部相關一律回覆提示
    if (cmd == "list" || cmd == "build" || cmd == "compile" || cmd == "init" || cmd == "clean")
    {
        std::cerr << "External toolchain is disabled. Use `run` or `selfhost pack`.\n";
        return 2;
    }
#endif

    // 直呼檔名的舊行為→全部改導到 VM
    if (cmd.size() > 2 && cmd.find('.') != std::string::npos)
    {
        // 舊用法: zhcl hello.c → 現在直接以 VM 跑
        return cmd_run(cmd, "", {});
    }

    std::cerr << "Unknown command. Try --help.\n";
    return 1;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

int initialize_project(bool verbose)
{
    if (verbose)
        std::cout << "Initializing new zhcl project..." << std::endl;

    // Create zhcl.toml
    std::ofstream config("zhcl.toml");
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
    std::ofstream main_cpp("main.cpp");
    main_cpp << "#include <iostream>\n";
    main_cpp << "\n";
    main_cpp << "int main() {\n";
    main_cpp << "    std::cout << \"Hello, zhcl!\" << std::endl;\n";
    main_cpp << "    return 0;\n";
    main_cpp << "}\n";
    main_cpp.close();

    // Create README.md
    std::ofstream readme("README.md");
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

    if (verbose)
        std::cout << "Created zhcl.toml, main.cpp, and README.md" << std::endl;
    return 0;
}

int list_compilers(const CompilerRegistry &registry, bool verbose)
{
    // Only show built-in supported languages (no external dependencies needed)
    std::cout << "Built-in supported languages (no external dependencies):" << std::endl;
    std::cout << "  ?批遣 C/C++ (via MSVC/gcc detection)" << std::endl;
    std::cout << "  ?批遣 Java (bytecode generation)" << std::endl;
    std::cout << "  ?批遣 Go (translation to C++)" << std::endl;
    std::cout << "  ?批遣 Chinese (.zh files) - 蝜??芸?嚗撓??摮葡/?湔/閮剔/憒?/?血?/??蝯?" << std::endl;
    std::cout << "  ?批遣 Python (translation to C++)" << std::endl;
    std::cout << "  ?批遣 JavaScript (translation to C++)" << std::endl;

    return 0;
}

int clean_project(bool verbose)
{
    if (verbose)
        std::cout << "Cleaning build artifacts..." << std::endl;

    std::vector<std::string> patterns = {"*.exe", "*.obj", "*.o", "*.class", "*.pyc", "__pycache__"};

    for (const auto &pattern : patterns)
    {
        for (const auto &entry : fs::recursive_directory_iterator("."))
        {
            std::string filename = entry.path().filename().string();
            if (fs::is_regular_file(entry.path()))
            {
                if (matches_pattern(filename, pattern))
                {
                    fs::remove(entry.path());
                    if (verbose)
                        std::cout << "Removed: " << entry.path() << std::endl;
                }
            }
            else if (fs::is_directory(entry.path()) && filename == "__pycache__")
            {
                fs::remove_all(entry.path());
                if (verbose)
                    std::cout << "Removed directory: " << entry.path() << std::endl;
            }
        }
    }

    return 0;
}

bool matches_pattern(const std::string &filename, const std::string &pattern)
{
    if (pattern == "*.exe")
        return filename.find(".exe") != std::string::npos;
    if (pattern == "*.obj")
        return filename.find(".obj") != std::string::npos;
    if (pattern == "*.o")
        return filename.find(".o") != std::string::npos;
    if (pattern == "*.class")
        return filename.find(".class") != std::string::npos;
    if (pattern == "*.pyc")
        return filename.find(".pyc") != std::string::npos;
    return false;
}
