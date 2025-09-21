
# 📘 ZHCL — 通用自然語言程式語法系統 (Universal Natural Language Programming Engine)

> ✅ 使用自然語言（繁體中文／英文）書寫程式  
> ✅ 自動翻譯為 C 程式語法，可交由 GCC / MSVC / Clang 編譯  
> ✅ 從 Hello World 到數學運算、流程控制都能支援  
> ✅ 不需要學 C 語法，只要會說中文就能寫邏輯

---

## 🧠 專案理念 | Project Vision

**把人說的話，變成電腦能理解的程式語言。**  
Turn what humans say into a language the machine can understand.

---

## ✨ 支援特性 | Features

- 中文 / 英文自然語言指令（不需學程式語法）  
  Chinese / English natural language commands (no need to learn syntax)
- 整數、小數、布林值、字串自動判斷與輸出  
  Auto type detection and output (int, float, bool, string)
- 數學函式支援（pow、sqrt、三角函數）  
  Math function support (pow, sqrt, trig...)
- Windows UTF-8 與 Unix Locale 支援  
  Console locale handling for Windows and Unix
- `_Generic` 自動輸出選擇  
  Type-based output dispatch via `_Generic`
- 容錯與語法回退（錯誤容忍）  
  Tolerant fallback mechanism for syntax and type errors

---

## 📂 專案結構 | Project Structure

```
include/     → 頭文件 Header files (e.g. chinese.h)
src/         → 翻譯器主程式 Compiler sources
examples/    → 範例程式 Examples (C + zh)
tools/       → 建構腳本 Build scripts for multiple platforms
```

---

## 🧪 快速體驗 | Quick Start

### hello.zh 範例:

```
主函式 開始
  輸出("你好，世界！")
結束
```

### 執行方式 / Run:

```bash
zhcl_new.exe examples/hello.zh
# 或 Or:
zhcc_cpp.exe examples/hello.zh -o hello.c
gcc hello.c -o hello.exe
```

---

## 📦 編譯教學 | Building

支援平台 Platforms:
- Windows (MSVC): build_windows_msvc.bat
- macOS (clang): build_clang_macos.sh
- Linux (GCC): build_gcc.sh

---

## 🧠 關鍵文件 | Key Files

- `chinese.h`: 中文自然語言對應核心 / CN natural language interface header  
- `zhcc_full.cpp`: CLI 語言翻譯器 CLI language processor  
- `zhcl_java.cpp`: 產生 Java Bytecode / Compile .class  
- `zhcl_new.cpp`: 萬用翻譯器 / universal translator  
- `examples/`: 範例程式 Examples  
- `tools/`: 建構腳本 Build tools  

---

## 🗣️ 語言擴充 | Language Pack Support

內建繁體中文，未來可支援：  
Built-in Traditional Chinese, extensible to:

- zh_cn (Simplified Chinese)  
- ja_jp (Japanese)  
- en_us (English)  
- fr_fr (French)

可用 `langpacks/xxx.langpack.json` 擴充語意  
Extend via `langpacks/xxx.langpack.json`.

---

## 📘 作者與授權 | Author & License

**作者 Author**: Ice Xu / RetryIX AGI  
**授權 License**: MIT License

🌍 ZHCL is an open semantic language compiler  
designed for transparent, multilingual logic execution.

All language packages, mappings, and extensions are  
openly licensed under [MIT / Custom OSL], and  
ZHCL strongly advocates **language rights and logic visibility**.

This project rejects closed adaptation, military repackaging,  
or authoritarian misuse of semantic logic.

Logic belongs to people. Language belongs to everyone.

---

## 🔥 最後的話

**程式不該只屬於工程師，它該屬於所有能思考的人。**  
**我們讓語言變成程式，讓抽象變成溝通，讓邏輯用自己的母語說出來。**
