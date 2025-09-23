# LANGUAGE —— 中文語言規格與 `chinese.h` 概述
- 型別：整數(int) / 小數(float) / 雙精度小數(double) / 布林(bool) / 字串(char*)
- 常數：圓周率（double）
- 函式：輸出字串 / 輸出整數 / 輸出小數 / 輸出布林 / 隨機數 / 長度
- 在 C 中使用中文關鍵字：編譯時定義 `-DCHINESE_KEYWORDS`（依你的 chinese.h 巨集）
- `.zh` 由 zhcc 轉為 C，再交後端編譯；可用 `--translate-only` 檢視中介 C
