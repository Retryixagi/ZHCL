#include "chinese.h"

void 計算器(void) {
    輸出字串("=== 簡單計算器 ===\n");

    輸出字串("輸入第一個數字：");
    double a = 輸入小數();

    輸出字串("輸入運算符 (+, -, *, /)：");
    char op;
    scanf(" %c", &op);

    輸出字串("輸入第二個數字：");
    double b = 輸入小數();

    double 結果;
    switch (op) {
        case '+':
            結果 = a + b;
            break;
        case '-':
            結果 = a - b;
            break;
        case '*':
            結果 = a * b;
            break;
        case '/':
            if (b != 0) {
                結果 = a / b;
            } else {
                輸出字串("錯誤：除零！\n");
                return;
            }
            break;
        default:
            輸出字串("無效運算符！\n");
            return;
    }

    輸出格式("結果：%.2f %c %.2f = %.2f\n", a, op, b, 結果);
}