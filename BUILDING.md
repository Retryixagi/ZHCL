# 建置 zhcc_cpp（C++ 前端）

## CMake
```
cmake -S . -B build
cmake --build build --config Release
```

## 直接編譯
- Windows (MSVC):
```
cl /std:c++17 /EHsc src\zhcc_cpp.cpp /I include /Fe:zhcc_cpp.exe
```
- Linux/macOS:
```
g++ -std=c++17 src/zhcc_cpp.cpp -I include -o zhcc_cpp
```

## 使用
- 只翻譯：
```
zhcc_cpp examples/hello.zh -o hello.c
```
- 翻譯後嘗試編譯（需要系統有 cl/cc）：
```
zhcc_cpp examples/hello.zh -o hello.exe --cc   # Windows
zhcc_cpp examples/hello.zh -o hello --cc       # Linux/macOS
```
