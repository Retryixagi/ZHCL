# CLI —— 指令列參考（精簡）
usage: zhcc.py [-h] [-o O] [-O OPT] [-I I] [-L L] [-l L] [--translate-only] [--selftest] [inputs ...] {introspect} ...
- `-o` 指定輸出（.obj/.o、.exe/.out 或 `.c` 搭配 `--translate-only`）
- `-O` 後端最佳化，如 -O2
- `-I/-L/-l` include/庫/連結庫
- `--translate-only`：僅輸出 C
- `--selftest`：自檢
introspect：
- `--introspect-list` / `introspect --list`
- `--introspect-show <名稱>` / `introspect --show <名稱>`
- `--introspect-search <關鍵字>` / `introspect --search <關鍵字>`
