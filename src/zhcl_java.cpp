// zhcl_java.cpp -- Generate JVM bytecode for Java files (no JDK needed)
// Implements route 3: self-generate .class files
// Windows: cl /std:c++17 /EHsc /utf-8 zhcl_java.cpp /Fe:zhcl_java.exe
// Linux/macOS: g++ -std:c++17 zhcl_java.cpp -o zhcl_java

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
using namespace std;

// 小心：全部 Big-Endian
struct W {
    std::vector<uint8_t> buf;
    void u1(uint8_t x){ buf.push_back(x); }
    void u2(uint16_t x){ buf.push_back((x>>8)&0xFF); buf.push_back(x&0xFF); }
    void u4(uint32_t x){ buf.push_back((x>>24)&0xFF); buf.push_back((x>>16)&0xFF);
                         buf.push_back((x>>8)&0xFF); buf.push_back(x&0xFF); }
    void bytes(const void* p, size_t n){ auto c=(const uint8_t*)p; buf.insert(buf.end(), c, c+n); }
    void utf8(const std::string& s){ u1(1); u2((uint16_t)s.size()); bytes(s.data(), s.size()); }
};

void emit_helloworld_class(std::vector<uint8_t>& out){
    W w;

    // 1) header
    w.u4(0xCAFEBABE); w.u2(0); w.u2(52);
    w.u2(28); // constant_pool_count

    // 2) 常量池（順序照上表）
    w.utf8("HelloWorld");              // #1
    w.u1(7); w.u2(1);                  // #2 Class -> #1
    w.utf8("java/lang/Object");        // #3
    w.u1(7); w.u2(3);                  // #4 Class -> #3
    w.utf8("java/lang/System");        // #5
    w.u1(7); w.u2(5);                  // #6 Class -> #5
    w.utf8("out");                     // #7
    w.utf8("Ljava/io/PrintStream;");   // #8
    w.u1(12); w.u2(7); w.u2(8);        // #9  NameAndType
    w.u1(9);  w.u2(6); w.u2(9);        // #10 Fieldref System.out
    w.utf8("java/io/PrintStream");     // #11
    w.u1(7);  w.u2(11);                // #12 Class -> #11
    w.utf8("println");                 // #13
    w.utf8("(Ljava/lang/String;)V");   // #14
    w.u1(12); w.u2(13); w.u2(14);      // #15 NameAndType
    w.u1(10); w.u2(12); w.u2(15);      // #16 Methodref PrintStream.println
    w.utf8("Hello, World!");             // #17
    w.u1(8);  w.u2(17);                // #18 String -> #17
    w.utf8("<init>");                  // #19
    w.utf8("()V");                     // #20
    w.u1(12); w.u2(19); w.u2(20);      // #21 NameAndType
    w.u1(10); w.u2(4);  w.u2(21);      // #22 Methodref Object.<init>
    w.utf8("Code");                    // #23
    w.utf8("main");                    // #24
    w.utf8("([Ljava/lang/String;)V");  // #25
    w.utf8("SourceFile");              // #26
    w.utf8("HelloWorld.java");         // #27

    // 3) 類別
    w.u2(0x0021);  // ACC_PUBLIC|ACC_SUPER
    w.u2(2);       // this_class #2
    w.u2(4);       // super_class #4
    w.u2(0);       // interfaces_count
    w.u2(0);       // fields_count

    // 4) methods_count = 2
    w.u2(2);

    // --- 方法 A: <init> ---
    w.u2(0x0001);    // public
    w.u2(19);        // name_index <init>
    w.u2(20);        // descriptor ()V
    w.u2(1);         // attributes_count = 1 (Code)
    w.u2(23);        // attribute_name_index = "Code"
    w.u4(17);        // attribute_length = 12 + code_length(5)
    w.u2(1);         // max_stack
    w.u2(1);         // max_locals
    w.u4(5);         // code_length
    w.u1(0x2a);      // aload_0
    w.u1(0xb7); w.u2(22); // invokespecial #22
    w.u1(0xb1);      // return
    w.u2(0);         // exception_table_length
    w.u2(0);         // attributes_count (inside Code)

    // --- 方法 B: main ---
    w.u2(0x0009);    // public static
    w.u2(24);        // "main"
    w.u2(25);        // "([Ljava/lang/String;)V"
    w.u2(1);         // attributes_count = 1 (Code)
    w.u2(23);        // Code
    w.u4(21);        // attribute_length = 12 + code_length(9)
    w.u2(2);         // max_stack
    w.u2(1);         // max_locals
    w.u4(9);         // code_length
    w.u1(0xb2); w.u2(10); // getstatic #10 System.out
    w.u1(0x12); w.u1(18); // ldc #18  "Hello"
    w.u1(0xb6); w.u2(16); // invokevirtual #16 println
    w.u1(0xb1);          // return
    w.u2(0);             // exception_table_length
    w.u2(0);             // attributes_count (inside Code)

    // 5) class attributes_count = 1 (SourceFile)
    w.u2(1);
    w.u2(26);  // "SourceFile"
    w.u4(2);   // attribute_length=2
    w.u2(27);  // sourcefile_index #27

    out.swap(w.buf);
}

class ByteWriter {
public:
    vector<uint8_t> data;
    void u1(uint8_t v) { data.push_back(v); }
    void u2(uint16_t v) { data.push_back(v >> 8); data.push_back(v & 0xFF); }
    void u4(uint32_t v) { u2(v >> 16); u2(v & 0xFFFF); }
    void bytes(const vector<uint8_t>& b) { data.insert(data.end(), b.begin(), b.end()); }
    void utf8(const string& s) {
        u2(s.size());
        for (char c : s) u1(c);
    }
};

vector<uint8_t> generate_helloworld_class() {
    vector<uint8_t> out;
    emit_helloworld_class(out);
    return out;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: zhcl_java HelloWorld.java" << endl;
        return 1;
    }

    string input = argv[1];
    if (input != "HelloWorld.java") {
        cout << "Only HelloWorld.java supported for now" << endl;
        return 1;
    }

    auto class_data = generate_helloworld_class();
    ofstream out("HelloWorld.class", ios::binary);
    out.write((char*)class_data.data(), class_data.size());
    out.close();

    cout << "Generated HelloWorld.class" << endl;

    // Run with java if available
    system("java HelloWorld");

    return 0;
}