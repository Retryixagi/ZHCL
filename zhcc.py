#!/usr/bin/env python3
"""
繁體中文編譯驅動器 - zhcc
將中文程式碼編譯為可執行文件
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

def find_backend():
    """查找可用的編譯器"""
    # 首先檢查具體路徑
    specific_paths = {
        'cl': 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Tools\\MSVC\\14.29.30133\\bin\\Hostx64\\x64\\cl.exe',
        'gcc': 'C:\\msys64\\mingw64\\bin\\gcc.exe'
    }

    for backend, path in specific_paths.items():
        if os.path.exists(path):
            return backend, path

    # 如果具體路徑找不到，則在PATH中查找
    for b in ['cl', 'gcc', 'clang']:
        path = shutil.which(b)
        if path:
            return b, path

    print("❌ 找不到編譯器 (cl.exe, gcc, 或 clang)")
    sys.exit(1)

def compile_with_backend(backend, backend_path, src, obj, includes):
    """使用指定後端編譯文件"""
    if backend == "cl":
        cmd = [backend_path, "/c", "/nologo", "/utf-8", str(src), f"/Fo{str(obj)}"]
        for inc in includes:
            cmd += ["/I", str(inc)]
        result = subprocess.run(cmd, text=True, capture_output=True)
    else:
        cmd = [backend_path, "-c", str(src), "-o", str(obj)]
        for inc in includes:
            cmd += ["-I", inc]
        result = subprocess.run(cmd, text=True, capture_output=True)

    return result.returncode == 0, result

def translate_compiler_errors(stderr, stdout):
    """翻譯編譯器錯誤訊息（簡化版）"""
    return stderr

class ChineseNLPProcessor:
    """繁體中文自然語言處理器 - 增強版"""

    def __init__(self):
        self.keyword_map = {
            # 數據類型
            '整數': 'int', '小數': 'double', '雙精度小數': 'double',
            '字符': 'char', '字串': 'char*', '布林': 'bool',

            # 控制結構
            '如果': 'if', '否則': 'else', '當': 'while', '對於': 'for',
            '則': '', '結束': '', '返回': 'return', '印出': 'printf',
            '輸出字串': 'printf', '輸出整數': 'printf', '輸出小數': 'printf',

            # 運算符
            '等於': '==', '大於': '>', '小於': '<', '大於等於': '>=', '小於等於': '<=',
            '不等於': '!=', '且': '&&', '或': '||', '非': '!',
            '加': '+', '減': '-', '乘': '*', '除': '/', '取餘': '%',
            '等於': '=',

            # 其他
            '真': 'true', '假': 'false', '空': 'NULL', '主函數': 'main'
        }

        # 變數名和函數名映射
        self.name_map = {
            '圓周率': '3.141592653589793',
            '半徑': 'radius', '面積': 'area', '結果': 'result',
            '年齡': 'age', '名字': 'name', '圓周率值': 'pi_value',
            '訊息': 'message', '問候': 'greet'
        }

        # 變數類型追蹤
        self.variable_types = {}  # 變數名 -> 類型

        # 函數映射
        self.function_map = {
            '隨機數': 'rand()',
            '平方根': 'sqrt',
            '絕對值': 'abs',
            '正弦': 'sin', '餘弦': 'cos', '正切': 'tan'
        }

    def translate_zh_to_c(self, zh_code, filename="<string>"):
        """將中文代碼轉換為C代碼"""
        lines = zh_code.split('\n')
        main_lines = []
        global_functions = []
        in_function = False
        current_function = []

        for line in lines:
            line = line.strip()

            # 跳過空行和註釋
            if not line or line.startswith('//'):
                if line.startswith('//'):
                    if in_function:
                        current_function.append(line)
                    else:
                        main_lines.append(line)
                continue

            # 處理函數定義 - 移到全局
            if line.startswith('函數 ') and '(' in line and not line.endswith(';'):
                in_function = True
                # 解析函數定義：函數 函數名(參數)
                func_def = line[3:].strip()  # 去掉 '函數 '
                # 替換參數類型
                func_def = func_def.replace('字串', 'char*')
                func_def = func_def.replace('整數', 'int')
                func_def = func_def.replace('小數', 'double')
                # 替換參數名稱
                for zh_name, c_name in self.name_map.items():
                    func_def = func_def.replace(zh_name, c_name)
                current_function = [f'void {func_def} {{']
                continue

            # 處理函數結束
            if line == '函數結束':
                if in_function:
                    current_function.append('}')
                    global_functions.extend(current_function)
                    current_function = []
                    in_function = False
                continue

            # 如果在函數內部，添加到函數體
            if in_function:
                # 處理函數內部的語句
                if line.startswith('印出') or line.startswith('輸出'):
                    print_stmt = self._parse_print_statement(line)
                    current_function.append(print_stmt)
                else:
                    c_line = self._translate_general_statement(line)
                    if c_line:
                        current_function.append(c_line)
                continue

            # 處理條件語句結束
            if line == '結束':
                main_lines.append('}')
                continue

            # 處理變數聲明
            var_declaration = self._parse_variable_declaration(line)
            if var_declaration:
                main_lines.append(var_declaration)
                continue

            # 處理條件語句
            if_condition = self._parse_if_condition(line)
            if if_condition:
                main_lines.append(if_condition)
                continue

            # 處理返回語句
            if line.startswith('返回'):
                return_stmt = self._parse_return_statement(line)
                main_lines.append(return_stmt)
                continue

            # 處理印出語句
            if line.startswith('印出') or line.startswith('輸出'):
                print_stmt = self._parse_print_statement(line)
                main_lines.append(print_stmt)
                continue

            # 處理一般語句
            c_line = self._translate_general_statement(line)
            if c_line:
                main_lines.append(c_line)

        # 組合C代碼
        c_code = '#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n#include <time.h>\n\n'

        # 添加全局函數
        if global_functions:
            c_code += '\n'.join(global_functions) + '\n\n'

        # 添加主函數
        c_code += 'int main() {\n' + \
                  '\n'.join(['    ' + line for line in main_lines if line.strip()]) + \
                  '\n    return 0;\n}'

        return c_code

        # 如果沒有主函數，添加基本結構
        if 'int main()' not in c_code:
            c_code = '#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n#include <time.h>\n\n' + \
                    'int main() {\n' + \
                    '\n'.join(['    ' + line for line in c_lines if line.strip()]) + \
                    '\n    return 0;\n}'

        return c_code

    def _parse_variable_declaration(self, line):
        """解析變數聲明：數據類型 變數名 = 值"""
        # 匹配模式：類型 變數名 = 值
        import re

        # 數據類型映射
        type_map = {
            '整數': 'int',
            '小數': 'double',
            '雙精度小數': 'double',
            '字符': 'char',
            '字串': 'char*',
            '布林': 'bool'
        }

        for zh_type, c_type in type_map.items():
            if line.startswith(zh_type + ' '):
                # 提取變數名和值
                rest = line[len(zh_type):].strip()
                if '=' in rest:
                    var_name, value = rest.split('=', 1)
                    var_name = var_name.strip()
                    value = value.strip()

                    # 處理變數名映射（使用完整匹配）
                    for zh_name, c_name in self.name_map.items():
                        if var_name == zh_name:
                            var_name = c_name
                            break

                    # 記錄變數類型
                    self.variable_types[var_name] = c_type

                    # 處理值中的常量
                    for zh_const, c_const in self.name_map.items():
                        if value == zh_const:
                            value = c_const
                            break

                    # 處理字串
                    if value.startswith('"') and value.endswith('"'):
                        if c_type == 'char*':
                            return f'{c_type} {var_name} = {value};'
                        else:
                            return f'{c_type} {var_name} = {value};'
                    else:
                        return f'{c_type} {var_name} = {value};'

        return None

    def _parse_if_condition(self, line):
        """解析條件語句：如果 條件 則"""
        if line.startswith('如果') and '則' in line:
            # 提取條件部分
            condition_part = line[2:-1].strip()  # 去掉 '如果' 和 '則'

            # 替換運算符
            for zh_op, c_op in self.keyword_map.items():
                condition_part = condition_part.replace(zh_op, c_op)

            # 處理變數名（完整匹配）
            for zh_name, c_name in self.name_map.items():
                if zh_name in condition_part:
                    condition_part = condition_part.replace(zh_name, c_name)

            return f'if ({condition_part}) {{'
        elif line == '否則':
            return '} else {'

        return None

    def _parse_return_statement(self, line):
        """解析返回語句：返回 值"""
        if line.startswith('返回'):
            value = line[2:].strip()
            if not value:
                return 'return 0;'
            else:
                # 處理變數名
                for zh_name, c_name in self.name_map.items():
                    value = value.replace(zh_name, c_name)
                return f'return {value};'

        return None

    def _parse_print_statement(self, line):
        """解析印出語句：印出 值"""
        if line.startswith('印出'):
            content = line[2:].strip()

            # 如果是字串，直接輸出
            if content.startswith('"') and content.endswith('"'):
                return f'printf({content});'
            else:
                # 處理變數名（完整匹配）
                var_name = content
                for zh_name, c_name in self.name_map.items():
                    if content == zh_name:
                        var_name = c_name
                        break

                # 根據變數類型確定格式
                if var_name in self.variable_types:
                    var_type = self.variable_types[var_name]
                    if var_type == 'int':
                        return f'printf("%d\\n", {var_name});'
                    elif var_type in ['double', 'float']:
                        return f'printf("%f\\n", {var_name});'
                    elif var_type == 'char*':
                        return f'printf("%s\\n", {var_name});'
                    else:
                        return f'printf("%s\\n", {var_name});'
                else:
                    # 推斷類型並添加格式
                    if '.' in content or content.replace('.', '').replace('-', '').isdigit():
                        return f'printf("%f\\n", {var_name});'
                    elif content.isdigit() or content.replace('-', '').isdigit():
                        return f'printf("%d\\n", {var_name});'
                    else:
                        return f'printf("%s\\n", {var_name});'

        elif line.startswith('輸出字串'):
            content = line[4:].strip()
            if content.startswith('"') and content.endswith('"'):
                return f'printf({content});'
        elif line.startswith('輸出整數'):
            var = line[4:].strip()
            for zh_name, c_name in self.name_map.items():
                if var == zh_name:
                    var = c_name
                    break
            return f'printf("%d\\n", {var});'
        elif line.startswith('輸出小數'):
            var = line[4:].strip()
            for zh_name, c_name in self.name_map.items():
                if var == zh_name:
                    var = c_name
                    break
            return f'printf("%f\\n", {var});'

        return None

    def _translate_general_statement(self, line):
        """處理一般語句"""
        # 保護字串中的內容不被替換
        import re

        # 找到所有字串
        strings = re.findall(r'"[^"]*"', line)
        placeholders = []
        for i, string in enumerate(strings):
            placeholder = f"__STRING_PLACEHOLDER_{i}__"
            placeholders.append((placeholder, string))
            line = line.replace(string, placeholder, 1)

        # 替換關鍵字
        for zh, c in self.keyword_map.items():
            line = line.replace(zh, c)

        # 替換變數名和函數名（避免替換字串中的內容）
        for zh, c in self.name_map.items():
            line = line.replace(zh, c)

        # 替換函數
        for zh_func, c_func in self.function_map.items():
            line = line.replace(zh_func, c_func)

        # 恢復字串
        for placeholder, original_string in placeholders:
            line = line.replace(placeholder, original_string)

        # 添加分號如果沒有
        if line and not line.endswith(';') and not line.endswith('{') and not line.endswith('}'):
            line += ';'

        return line

def translate_zh_to_c(zh_code, filename="<string>"):
    """全局翻譯函數"""
    processor = ChineseNLPProcessor()
    return processor.translate_zh_to_c(zh_code, filename)

def main():
    """主函數 - 簡化版中文編譯器"""
    ap = argparse.ArgumentParser(description="繁體中文編譯驅動器 - zhcc")
    ap.add_argument("inputs", nargs="*", help="輸入的 .zh 源文件")
    ap.add_argument("-o", default="a.out", help="輸出可執行文件")
    ap.add_argument("--translate-only", action="store_true", help="只翻譯不編譯，輸出C代碼")
    ap.add_argument("--selftest", action="store_true", help="運行自檢測試")

    args = ap.parse_args()

    # 自檢測試
    if args.selftest:
        print("🧪 運行 zhcc 自檢測試...")
        try:
            backend, backend_path = find_backend()
            print(f"✅ 編譯器檢測通過: {backend} -> {backend_path}")
            print("✅ 自檢通過")
            return
        except SystemExit as e:
            print(f"❌ 自檢失敗: {e}")
            return

    # 檢查輸入文件
    if not args.inputs:
        ap.print_help()
        return

    # 只翻譯模式
    if args.translate_only:
        for inp in args.inputs:
            zh_path = Path(inp)
            if not zh_path.exists():
                print(f"❌ 找不到源文件: {inp}")
                sys.exit(1)

            print(f"📝 翻譯源文件: {inp}")
            # 讀取文件
            text = None
            for encoding in ["utf-8", "utf-16", "gbk"]:
                try:
                    text = zh_path.read_text(encoding=encoding)
                    break
                except UnicodeDecodeError:
                    continue
            if text is None:
                text = zh_path.read_text(encoding="utf-8", errors="replace")

            c_text = translate_zh_to_c(text, str(zh_path))
            print("=== 生成的C代碼 ===")
            print(c_text)
        return

    # 檢測編譯器
    backend, backend_path = find_backend()

    # 編譯所有文件
    with tempfile.TemporaryDirectory() as td:
        objs = []
        out_path = Path(args.o)

        for inp in args.inputs:
            inp_path = Path(inp)
            if not inp_path.exists():
                print(f"❌ 找不到源文件: {inp}")
                sys.exit(1)

            print(f"📝 處理源文件: {inp}")

            # 讀取並翻譯中文文件
            text = None
            for encoding in ["utf-8", "utf-16", "gbk"]:
                try:
                    text = inp_path.read_text(encoding=encoding)
                    break
                except UnicodeDecodeError:
                    continue
            if text is None:
                text = inp_path.read_text(encoding="utf-8", errors="replace")

            c_text = translate_zh_to_c(text, str(inp_path))

            # 寫入臨時C文件
            c_path = Path(td) / (inp_path.stem + ".c")
            c_path.write_text(c_text, encoding="utf-8")

            # 編譯為物件文件
            obj = Path(td) / (inp_path.stem + (".obj" if backend == "cl" else ".o"))

            success, message = compile_with_backend(backend, backend_path, str(c_path), str(obj), [])
            if not success:
                print("❌ 編譯失敗")
                print("---STDOUT---")
                print(message.stdout)
                print("---STDERR---")
                print(message.stderr)
                sys.exit(1)

            objs.append(obj)

        # 連結
        exe = out_path if out_path.suffix.lower() in (".exe", ".dll") else Path("a.out")

        if backend == "cl":
            # MSVC 連結
            cmd = [backend_path, "/nologo"] + [str(obj) for obj in objs] + [f"/Fe{str(exe)}"]
            result = subprocess.run(cmd, text=True, capture_output=True)
        else:
            # GCC/Clang 連結
            cmd = [backend_path] + [str(obj) for obj in objs] + ["-o", str(exe)]
            result = subprocess.run(cmd, text=True, capture_output=True)

        if result.returncode != 0:
            print("❌ 連結失敗")
            print("---STDOUT---")
            print(result.stdout)
            print("---STDERR---")
            print(result.stderr)
            sys.exit(1)

        print(f"✅ 成功輸出: {exe}")

if __name__ == "__main__":
    main()