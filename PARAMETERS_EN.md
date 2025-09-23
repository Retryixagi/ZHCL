# zhcl Detailed Parameter Usage Guide

## Compiler Standards Compatibility

The zhcl system has excellent modern standards compatibility:

### C Language Standards

- **C11**: Fully supported
- **C17**: Fully supported
- **C23**: Partially supported (depending on backend compiler capabilities)

### C++ Language Standards

- **C++17**: Fully supported
- **C++20**: Fully supported
- **C++23**: Partially supported (depending on backend compiler capabilities)

### Automatic Adaptation

The system automatically selects the most suitable standard version based on detected compiler capabilities:

- MSVC: `/std:c17` or `/std:c++17`
- GCC/Clang: `-std=c17` or `-std=c++17`

### Encoding Support

- **UTF-8**: Native support, no special flags needed
- **Wide Characters**: Full Unicode support
- **Multi-byte Characters**: Complete compatibility

### Generated Code Compatibility Strategy

The zhcl system uses a **backward compatibility strategy**, generating code using base standards to ensure maximum compatibility:

#### Target Standard for Generated Code

- **Target Standard**: Base C89/C90 standard code
- **Headers**: Standard C library (`<stdio.h>`, `<stdlib.h>`, `<math.h>`)
- **Syntax**: Traditional C syntax, avoiding modern language extensions

#### Compatibility Advantages

- **Wide Support**: Compilable on any C-supporting compiler
- **Cross-platform**: Windows, Linux, macOS, embedded systems
- **Backward Compatible**: Supports ancient compiler versions

#### Example Generated Code

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

## Basic Syntax

```bash
zhcl <command> [subcommand] [options] [file] [-- vm_args...]
```

## Command Details

### 1. Run Command

Execute files directly through the virtual machine, supporting multiple languages.

**Syntax:**

```bash
zhcl run <file> [options]
```

**Parameters:**

- `<file>`: Source file to run, supported extensions: `.zh`, `.c`, `.cpp`, `.js`

**Options:**

- `--frontend=<name>`: Force specific language frontend
  - `zh`: Chinese programming language
  - `c-lite`: Simplified C language
  - `cpp-lite`: Simplified C++ language
  - `js-lite`: Simplified JavaScript

**Examples:**

```bash
# Run Chinese program
zhcl run hello.zh

# Force C-lite frontend for C file
zhcl run --frontend=c-lite hello.c

# Pass parameters to virtual machine
zhcl run script.zh -- 2025 3 20 16 0 0 -5
```

### 2. Selfhost Commands

Generate self-contained executables with no external dependencies.

#### 2.1 Pack

Package source code into a self-contained executable.

**Syntax:**

```bash
zhcl selfhost pack <input_file> -o <output_file> [options]
```

**Parameters:**

- `<input_file>`: Input source file, supports: `.js`, `.py`, `.go`, `.java`, `.zh`
- `-o <output_file>`: Output executable file path (usually `.exe`)

**Examples:**

```bash
# Package JavaScript file
zhcl selfhost pack hello.js -o hello.exe

# Package Chinese program
zhcl selfhost pack hello.zh -o hello.exe

# Package Python program
zhcl selfhost pack script.py -o script.exe
```

#### 2.2 Verify

Verify the integrity of self-contained executables.

**Syntax:**

```bash
zhcl selfhost verify <executable_file>
```

**Parameters:**

- `<executable_file>`: Executable file to verify

**Examples:**

```bash
zhcl selfhost verify hello.exe
# Output: [selfhost] payload v1 found size=XXX crc=OK
```

#### 2.3 Explain

Display bytecode disassembly information for source code.

**Syntax:**

```bash
zhcl selfhost explain <input_file>
```

**Parameters:**

- `<input_file>`: Source file to analyze

**Examples:**

```bash
zhcl selfhost explain hello.zh
# Displays bytecode disassembly information
```

### 3. List Frontends

List all available language frontends.

**Syntax:**

```bash
zhcl list-frontends
```

**Example Output:**

```
Available frontends:
- zh: Chinese programming language
- c-lite: Simplified C
- cpp-lite: Simplified C++
- js-lite: Simplified JavaScript
```

### 4. Help

Display help information.

**Syntax:**

```bash
zhcl --help
zhcl -h
```

## General Options

### --frontend=<name>

Force the use of a specific language frontend.

**Available Values:**

- `zh`: Chinese programming language
- `c-lite`: Simplified C language
- `cpp-lite`: Simplified C++ language
- `js-lite`: Simplified JavaScript

**Examples:**

```bash
zhcl run --frontend=zh script.c  # Run C file as Chinese program
```

### -- <vm_args...>

Pass parameters to the virtual machine. Parameters are stored as integers in VM slots (slot 0, 1, 2, ...).

**Examples:**

```bash
# Pass year, month, day, hour, minute, second, timezone parameters
zhcl run script.zh -- 2025 3 20 16 0 0 -5
```

## Environment Variables

### ZHCL_SELFHOST_QUIET

Controls the output behavior of selfhost executables.

**Values:**

- `0` or not set: Display banner information (default)
- `1`: Silent mode, no banner display

**Examples:**

```bash
# Windows
set ZHCL_SELFHOST_QUIET=1 && .\hello.exe

# Linux/macOS
ZHCL_SELFHOST_QUIET=1 ./hello.exe
```

## Supported File Types

| Extension                 | Language            | Compilation Method | Selfhost Support | Description                  |
| ------------------------- | ------------------- | ------------------ | ---------------- | ---------------------------- |
| `.zh`                     | Chinese Programming | Bytecode Execution | ✅               | Full Chinese keyword support |
| `.c`                      | C                   | Native Compilation | ❌               | Standard C language          |
| `.cpp`<br>`.cc`<br>`.cxx` | C++                 | Native Compilation | ❌               | Standard C++ language        |
| `.java`                   | Java                | Native Compilation | ✅               | Standard Java language       |
| `.py`                     | Python              | Transpiled to C++  | ✅               | Python 2/3 syntax            |
| `.go`                     | Go                  | Transpiled to C++  | ✅               | Go language syntax           |
| `.js`                     | JavaScript          | Bytecode Execution | ✅               | Standard JavaScript          |

## Error Handling

### Common Errors

1. **"read fail: <file>"**

   - Cause: File does not exist or path is incorrect
   - Solution: Check file path and name

2. **"Unknown frontend: <name>"**

   - Cause: Specified frontend does not exist
   - Solution: Use `list-frontends` to view available frontends

3. **"Unsupported file extension"**
   - Cause: File extension not supported
   - Solution: Check supported file types list

## Advanced Usage

### Batch Compilation

```batch
@echo off
for %%f in (*.zh) do (
    echo Compiling %%f...
    zhcl selfhost pack "%%f" -o "%%~nf.exe"
)
echo Complete!
```

### Conditional Compilation

```bash
# Select compilation method based on file type
if [[ $1 == *.zh ]]; then
    zhcl selfhost pack "$1" -o "${1%.zh}.exe"
elif [[ $1 == *.js ]]; then
    zhcl selfhost pack "$1" -o "${1%.js}.exe"
else
    echo "Unsupported file type"
fi
```

## Performance Considerations

- **Selfhost Executables**: Packaged files contain complete runtime, larger but no external dependencies
- **Direct Execution**: Run through VM, smaller files but requires zhcl to be present
- **Compilation Optimization**: For large projects, selfhost packaging is recommended for better distribution convenience

## Troubleshooting

### Compilation Failures

1. Check if file syntax is correct
2. Ensure file encoding is UTF-8
3. Check if file path contains special characters

### Runtime Failures

1. Ensure all dependency files exist
2. Check file permissions
3. Confirm system supports UTF-8 encoding

### Corrupted Selfhost Files

1. Use `selfhost verify` to check file integrity
2. Repackage source files
3. Check if disk space is sufficient

---

_This document covers all parameters and usage methods for zhcl v1.0. For issues, please refer to the source code or submit an issue report._
