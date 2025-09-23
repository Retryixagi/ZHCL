# zhcl 詳細參數使用說明

## 編譯器標準兼容性

zhcl 系統具有出色的現代標準兼容性：

### C 語言標準

- **C11**: 完全支持
- **C17**: 完全支持
- **C23**: 部分支持（根據後端編譯器能力）

### C++ 語言標準

- **C++17**: 完全支持
- **C++20**: 完全支持
- **C++23**: 部分支持（根據後端編譯器能力）

### 自動適配

系統會根據檢測到的編譯器能力自動選擇最適合的標準版本：

- MSVC: `/std:c17` 或 `/std:c++17`
- GCC/Clang: `-std=c17` 或 `-std=c++17`

### 編碼支持

- **UTF-8**: 原生支持，不需要特殊標記
- **寬字符**: 完全支持 Unicode
- **多字節字符**: 完整兼容

### 產物代碼兼容性策略

zhcl 系統採用**向下兼容策略**，產生的代碼使用基礎標準以確保最大兼容性：

#### 轉譯目標標準

- **目標標準**: 基礎 C89/C90 標準代碼
- **頭文件**: 標準 C 庫 (`<stdio.h>`, `<stdlib.h>`, `<math.h>`)
- **語法**: 傳統 C 語法，避免現代語言擴展

#### 兼容性優勢

- **廣泛支持**: 能在任何支持 C 的編譯器上編譯
- **跨平台**: Windows、Linux、macOS、嵌入式系統
- **向後兼容**: 支持古老的編譯器版本

#### 範例產物代碼

```c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

int main() {
    printf("Hello from Chinese!");
    int x = 42;
    printf("%d\n", x);
    return 0;
}
```

## 基本語法

```bash
zhcl <command> [subcommand] [options] [file] [-- vm_args...]
```

## 命令詳解

### 1. 運行命令 (run)

直接通過虛擬機運行文件，支持多種語言。

**語法：**

```bash
zhcl run <file> [options]
```

**參數說明：**

- `<file>`: 要運行的源文件，支持的擴展名：`.zh`, `.c`, `.cpp`, `.js`

**選項：**

- `--frontend=<name>`: 強制指定前端語言
  - `zh`: 中文編程語言
  - `c-lite`: 簡化 C 語言
  - `cpp-lite`: 簡化 C++語言
  - `js-lite`: 簡化 JavaScript

**範例：**

```bash
# 運行中文程序
zhcl run hello.zh

# 強制使用C-lite前端運行C文件
zhcl run --frontend=c-lite hello.c

# 傳遞參數給虛擬機
zhcl run script.zh -- 2025 3 20 16 0 0 -5
```

### 2. 自宿主命令 (selfhost)

生成自包含的可執行文件，無需外部依賴。

#### 2.1 打包 (pack)

將源代碼打包成自包含的可執行文件。

**語法：**

```bash
zhcl selfhost pack <input_file> -o <output_file> [options]
```

**參數說明：**

- `<input_file>`: 輸入源文件，支持：`.js`, `.py`, `.go`, `.java`, `.zh`
- `-o <output_file>`: 輸出可執行文件路徑（通常為 `.exe`）

**範例：**

```bash
# 打包JavaScript文件
zhcl selfhost pack hello.js -o hello.exe

# 打包中文程序
zhcl selfhost pack hello.zh -o hello.exe

# 打包Python程序
zhcl selfhost pack script.py -o script.exe
```

#### 2.2 驗證 (verify)

驗證自包含可執行文件的完整性。

**語法：**

```bash
zhcl selfhost verify <executable_file>
```

**參數說明：**

- `<executable_file>`: 要驗證的可執行文件

**範例：**

```bash
zhcl selfhost verify hello.exe
# 輸出: [selfhost] payload v1 found size=XXX crc=OK
```

#### 2.3 解釋 (explain)

顯示源代碼的位元碼反匯編信息。

**語法：**

```bash
zhcl selfhost explain <input_file>
```

**參數說明：**

- `<input_file>`: 要分析的源文件

**範例：**

```bash
zhcl selfhost explain hello.zh
# 顯示位元碼反匯編信息
```

### 3. 前端列表 (list-frontends)

列出所有可用的語言前端。

**語法：**

```bash
zhcl list-frontends
```

**輸出範例：**

```
Available frontends:
- zh: Chinese programming language
- c-lite: Simplified C
- cpp-lite: Simplified C++
- js-lite: Simplified JavaScript
```

### 4. 幫助 (help)

顯示幫助信息。

**語法：**

```bash
zhcl --help
zhcl -h
```

## 通用選項

### --frontend=<name>

強制指定使用的語言前端。

**可用值：**

- `zh`: 中文編程語言
- `c-lite`: 簡化 C 語言
- `cpp-lite`: 簡化 C++語言
- `js-lite`: 簡化 JavaScript

**範例：**

```bash
zhcl run --frontend=zh script.c  # 將C文件作為中文程序運行
```

### -- <vm_args...>

向虛擬機傳遞參數。參數將作為整數存儲在 VM 的插槽中（slot 0, 1, 2, ...）。

**範例：**

```bash
# 傳遞年月日時分秒時區參數
zhcl run script.zh -- 2025 3 20 16 0 0 -5
```

## 環境變數

### ZHCL_SELFHOST_QUIET

控制自宿主可執行文件的輸出行為。

**值：**

- `0` 或未設置：顯示橫幅信息（預設）
- `1`: 靜默模式，不顯示橫幅

**範例：**

```bash
# Windows
set ZHCL_SELFHOST_QUIET=1 && .\hello.exe

# Linux/macOS
ZHCL_SELFHOST_QUIET=1 ./hello.exe
```

## 支持的文件類型

| 擴展名                    | 語言       | 編譯方式   | 自宿主支持 | 說明                 |
| ------------------------- | ---------- | ---------- | ---------- | -------------------- |
| `.zh`                     | 中文編程   | 位元碼執行 | ✅         | 完整的中文關鍵字支持 |
| `.c`                      | C          | 原生編譯   | ❌         | 標準 C 語言          |
| `.cpp`<br>`.cc`<br>`.cxx` | C++        | 原生編譯   | ❌         | 標準 C++語言         |
| `.java`                   | Java       | 原生編譯   | ✅         | 標準 Java 語言       |
| `.py`                     | Python     | 轉譯為 C++ | ✅         | Python 2/3 語法      |
| `.go`                     | Go         | 轉譯為 C++ | ✅         | Go 語言語法          |
| `.js`                     | JavaScript | 位元碼執行 | ✅         | 標準 JavaScript      |

## 錯誤處理

### 常見錯誤

1. **"read fail: <file>"**

   - 原因：文件不存在或路徑錯誤
   - 解決：檢查文件路徑和名稱

2. **"Unknown frontend: <name>"**

   - 原因：指定的前端不存在
   - 解決：使用 `list-frontends` 查看可用前端

3. **"Unsupported file extension"**
   - 原因：文件擴展名不受支持
   - 解決：檢查支持的文件類型列表

## 進階用法

### 批處理編譯

```batch
@echo off
for %%f in (*.zh) do (
    echo 編譯 %%f...
    zhcl selfhost pack "%%f" -o "%%~nf.exe"
)
echo 完成！
```

### 條件編譯

```bash
# 根據文件類型選擇編譯方式
if [[ $1 == *.zh ]]; then
    zhcl selfhost pack "$1" -o "${1%.zh}.exe"
elif [[ $1 == *.js ]]; then
    zhcl selfhost pack "$1" -o "${1%.js}.exe"
else
    echo "不支持的文件類型"
fi
```

## 性能考慮

- **自宿主執行文件**：打包後的文件包含完整的運行時，文件較大但無外部依賴
- **直接運行**：通過 VM 運行，文件小但需要 zhcl 存在
- **編譯優化**：對於大型項目，建議使用自宿主打包以提高分發便利性

## 故障排除

### 編譯失敗

1. 檢查文件語法是否正確
2. 確認文件編碼為 UTF-8
3. 檢查文件路徑是否包含特殊字符

### 運行失敗

1. 確保所有依賴文件存在
2. 檢查文件權限
3. 確認系統支持 UTF-8 編碼

### 自宿主文件損壞

1. 使用 `selfhost verify` 檢查文件完整性
2. 重新打包源文件
3. 檢查磁盤空間是否充足

---

_本文檔涵蓋 zhcl v1.0 的所有參數和使用方法。如有問題，請參考原始碼或提交問題報告。_
