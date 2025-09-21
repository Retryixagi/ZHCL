#include "chinese.h"

void 計算器(void);
void 隨機數生成器(void);
void 巨石陣太陽模擬(void);
void 羽蛇神單次模擬(void);
void 掃描羽蛇神最佳時刻(int Y,int M,int D,double tz,int sh,int eh,int step);

int main(void) {
    初始化中文環境();

    輸出字串("=== 中文跨平台示範程式 ===\n");
    輸出字串("1. 計算器\n");
    輸出字串("2. 隨機數生成器\n");
    輸出字串("3. 巨石陣太陽模擬\n");
    輸出字串("4. 羽蛇神神殿蛇影模擬\n");
    輸出字串("5. 羽蛇神最佳時刻掃描（春分）\n");
    輸出字串("6. 中文編譯器演示\n");
    輸出字串("0. 退出\n");
    輸出字串("請選擇：");

    int 選擇 = 輸入整數();

    switch (選擇) {
        case 1:
            計算器();
            break;
        case 2:
            隨機數生成器();
            break;
        case 3:
            巨石陣太陽模擬();
            break;
        case 4:
            羽蛇神單次模擬();
            break;
        case 5:
            // 春分附近的最佳時刻掃描（當地時區 -5）
            掃描羽蛇神最佳時刻(2025, 3, 20, -5, 15, 18, 30);
            break;
        case 6:
            輸出字串("=== 中文編譯器演示 ===\n");
            輸出字串("請將 .zh 文件放在專案目錄中，然後使用命令行:\n");
            輸出字串("python zhcc.py hello.zh -o hello.exe\n");
            輸出字串("或只翻譯: python zhcc.py --translate-only hello.zh\n");
            輸出字串("查看符號: python zhcc.py --introspect-list\n");
            break;
        case 0:
            輸出字串("再見！\n");
            break;
        default:
            輸出字串("無效選擇。\n");
            break;
    }

    return 0;
}