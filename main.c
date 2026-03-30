#include <graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROWS 4
#define COLS 8
#define GRID_SIZE 80

typedef struct {
    int id;
    int isFaceUp;
    int color;
    char name[10];
} Piece;

Piece board[ROWS][COLS];

// --- 羅馬拼音棋子清單 ---
// 前 7 個為黑方 (BLACK)，後 7 個為紅方 (RED)
const char* pieceNames[] = {
    "Jiang", "Shi", "Xiang", "Ju", "Ma", "Pao", "Zu",  // 黑方 (Black)
    "Shuai", "Shi", "Xiang", "Che", "Ma", "Pao", "Bing" // 紅方 (Red)
};

void initGame() {
    int pieces[32], i, r, c, k = 0;

    // 分配 32 顆棋子 ID
    for(i = 0; i < 14; i++) {
        pieces[k++] = i; pieces[k++] = i;
    }
    while(k < 32) {
        pieces[k++] = (k % 2 == 0) ? 6 : 13; // 補足卒與兵
    }

    // 洗牌 (Shuffle)
    for (i = 0; i < 32; i++) {
        int t = rand() % 32;
        int temp = pieces[i]; pieces[i] = pieces[t]; pieces[t] = temp;
    }

    k = 0;
    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            board[r][c].id = pieces[k++];
            board[r][c].isFaceUp = 0;
            // ID 0-6 是黑方，7-13 是紅方
            board[r][c].color = (board[r][c].id < 7) ? BLACK : RED;
            sprintf(board[r][c].name, "%s", pieceNames[board[r][c].id]);
        }
    }
}

void draw() {
    int r, c;
    setbkcolor(BLACK);
    cleardevice();

    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            int x1 = c * GRID_SIZE, y1 = r * GRID_SIZE;

            // 畫白色格線
            setcolor(WHITE);
            rectangle(x1, y1, x1 + GRID_SIZE, y1 + GRID_SIZE);

            if (board[r][c].isFaceUp) {
                // 1. 畫棋子底圓
                setfillstyle(SOLID_FILL, WHITE);
                fillellipse(x1 + 40, y1 + 40, 35, 35); // 稍微放大圓圈

                // 2. 設定字體：TRIPLEX_FONT (粗體向量字), 大小建議設為 1 或 2
                // 注意：大小設為 1 在某些系統就很清楚，2 會非常大
                settextstyle(TRIPLEX_FONT, HORIZ_DIR, 1);

                setbkcolor(WHITE);          // 背景與圓圈一致
                setcolor(board[r][c].color); // 紅色或黑色

                // 3. 計算字體寬度以達成「自動置中」
                // textwidth() 可以獲取字串在當前字型下的像素寬度
                int tw = textwidth(board[r][c].name);
                int th = textheight(board[r][c].name);

                // 核心邏輯：格中心 (40) 減去 字寬的一半
                int targetX = x1 + (GRID_SIZE - tw) / 2;
                int targetY = y1 + (GRID_SIZE - th) / 2;

                outtextxy(targetX, targetY, board[r][c].name);

            } else {
                // 蓋住時顯示深灰色方塊 (中心畫一個小 X 代表背面)
                setfillstyle(SOLID_FILL, DARKGRAY);
                bar(x1 + 5, y1 + 5, x1 + GRID_SIZE - 5, y1 + GRID_SIZE - 5);
                setcolor(LIGHTGRAY);
                line(x1+30, y1+30, x1+50, y1+50); // 背面裝飾線
                line(x1+50, y1+30, x1+30, y1+50);
            }
        }
    }
}

// ... main 函數維持不變

int main() {
    srand((unsigned int)time(NULL));
    initwindow(COLS * GRID_SIZE, ROWS * GRID_SIZE, "Dark Chess - C_PROJECT");
    initGame();
    while (1) {
        draw();
        int px, py;
        while (!ismouseclick(WM_LBUTTONDOWN)) delay(10);
        getmouseclick(WM_LBUTTONDOWN, px, py);
        int pc = px / GRID_SIZE;
        int pr = py / GRID_SIZE;
        if (pr < ROWS && pc < COLS && !board[pr][pc].isFaceUp) {
            board[pr][pc].isFaceUp = 1;
            draw();
            delay(500);
            int cr, cc;
            do { cr = rand() % ROWS; cc = rand() % COLS; } while (board[cr][cc].isFaceUp);
            board[cr][cc].isFaceUp = 1;
        }
    }
    return 0;
}
