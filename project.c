#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define ROWS 4
#define COLS 8

int board[ROWS][COLS];
int revealed[ROWS][COLS];
const char* names[] = {"", "K", "G", "E", "R", "H", "C", "S", "k", "g", "e", "r", "h", "c", "s"};

int state = 0; // 0: 選擇先後手, 1: 遊戲中, 2: 遊戲結束
int turn = 0;  // 0: Player, 1: Computer
int playerColor = 0; // 0: 未定, 1: 紅, 2: 黑
int compColor = 0;   // 0: 未定, 1: 紅, 2: 黑
int playerMoves = 0;
int compMoves = 0;
int selR = -1, selC = -1; // 玩家選中的棋子

// 判斷棋子陣營 (回傳 1=紅, 2=黑, 0=空)
int getPieceColor(int p) {
    if (p == 0) return 0;
    return (p <= 7) ? 1 : 2;
}

int isPlayerPiece(int p) { return getPieceColor(p) == playerColor; }
int isCompPiece(int p) { return getPieceColor(p) == compColor; }

// 計算兩格之間的棋子數量 (給炮/包跳吃用)
int countPiecesBetween(int r1, int c1, int r2, int c2) {
    int count = 0;
    if (r1 == r2) {
        int minC = (c1 < c2) ? c1 : c2;
        int maxC = (c1 > c2) ? c1 : c2;
        for (int c = minC + 1; c < maxC; c++) if (board[r1][c] != 0) count++;
        return count;
    } else if (c1 == c2) {
        int minR = (r1 < r2) ? r1 : r2;
        int maxR = (r1 > r2) ? r1 : r2;
        for (int r = minR + 1; r < maxR; r++) if (board[r][c1] != 0) count++;
        return count;
    }
    return -1;
}

// 檢查移動或吃子是否合法
int isValidMove(int r1, int c1, int r2, int c2) {
    if (board[r1][c1] == 0) return 0;
    int p1 = board[r1][c1];
    int p2 = board[r2][c2]; 
    
    if (p2 != 0 && !revealed[r2][c2]) return 0;
    if (p2 != 0 && getPieceColor(p1) == getPieceColor(p2)) return 0;

    int rA = (p1 > 7) ? p1 - 7 : p1; 
    int rB = (p2 > 7) ? p2 - 7 : p2; 
    int dist = abs(r1 - r2) + abs(c1 - c2);

    if (rA == 6) { 
        if (p2 == 0) return dist == 1; 
        else return countPiecesBetween(r1, c1, r2, c2) == 1; 
    } else { 
        if (dist != 1) return 0; 
        if (p2 == 0) return 1;   
        if (rA == 1 && rB == 7) return 0; 
        if (rA == 7 && rB == 1) return 1; 
        return rA <= rB; 
    }
}

int isThreatened(int r, int c) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] != 0 && revealed[i][j] && isPlayerPiece(board[i][j])) {
                if (isValidMove(i, j, r, c)) return 1;
            }
        }
    }
    return 0;
}

void checkGameOver() {
    if (playerMoves >= 10 && compMoves >= 10) state = 2;
}

// 電腦回合邏輯
void computerTurnTimer(int value) {
    if (state != 1 || turn != 1 || compMoves >= 10) return;

    int moved = 0;
    
    // 1. 避開危險
    if (!moved && compColor != 0) {
        for (int i = 0; i < ROWS && !moved; i++) {
            for (int j = 0; j < COLS && !moved; j++) {
                if (board[i][j] != 0 && revealed[i][j] && isCompPiece(board[i][j])) {
                    if (isThreatened(i, j)) { 
                        int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
                        for (int d = 0; d < 4; d++) {
                            int nr = i + dirs[d][0], nc = j + dirs[d][1];
                            if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && board[nr][nc] == 0) {
                                if (!isThreatened(nr, nc)) {
                                    board[nr][nc] = board[i][j]; revealed[nr][nc] = 1;
                                    board[i][j] = 0; revealed[i][j] = 0;
                                    moved = 1; break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 2. 吃掉玩家棋子
    if (!moved && compColor != 0) {
        for (int i = 0; i < ROWS && !moved; i++) {
            for (int j = 0; j < COLS && !moved; j++) {
                if (board[i][j] != 0 && revealed[i][j] && isCompPiece(board[i][j])) {
                    for (int tr = 0; tr < ROWS && !moved; tr++) {
                        for (int tc = 0; tc < COLS && !moved; tc++) {
                            if (board[tr][tc] != 0 && revealed[tr][tc] && isPlayerPiece(board[tr][tc])) {
                                if (isValidMove(i, j, tr, tc)) {
                                    board[tr][tc] = board[i][j]; revealed[tr][tc] = 1;
                                    board[i][j] = 0; revealed[i][j] = 0;
                                    moved = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 3. 隨機翻牌
    if (!moved) {
        int unrev[32][2], count = 0;
        for (int i=0; i<ROWS; i++) 
            for (int j=0; j<COLS; j++) 
                if (!revealed[i][j] && board[i][j] != 0) {
                    unrev[count][0] = i; unrev[count][1] = j; count++;
                }
        
        if (count > 0) {
            int idx = rand() % count;
            int r = unrev[idx][0], c = unrev[idx][1];
            revealed[r][c] = 1;
            if (compColor == 0) {
                compColor = getPieceColor(board[r][c]);
                playerColor = (compColor == 1) ? 2 : 1;
            }
            moved = 1;
        }
    }

    compMoves++;
    turn = 0; // 換玩家
    checkGameOver();
    glutPostRedisplay();
}

void initBoard() {
    int all[32] = {1,2,2,3,3,4,4,5,5,6,6,7,7,7,7,7, 8,9,9,10,10,11,11,12,12,13,13,14,14,14,14,14};
    srand(time(NULL));
    for (int i=0; i<32; i++) {
        int r = rand()%32;
        int t = all[i]; all[i] = all[r]; all[r] = t;
    }
    int k=0;
    for (int i=0; i<ROWS; i++) 
        for (int j=0; j<COLS; j++) { board[i][j]=all[k++]; revealed[i][j]=0; }
}

// 支援自訂顏色的文字繪製
void drawText(float x, float y, const char* s, float r, float g, float b, void* font) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(font, *s++);
}

// 繪製棋子形狀與邊框的輔助函式
void drawPieceRect(float x, float y, float w, float h, float r, float g, float b, int isSelected) {
    // 內部填色
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x+w, y);
        glVertex2f(x+w, y-h);
        glVertex2f(x, y-h);
    glEnd();

    // 邊框與高亮
    glLineWidth(isSelected ? 4.0 : 2.0);
    if (isSelected) glColor3f(1.0, 0.85, 0.0); // 金黃色高亮
    else glColor3f(0.3, 0.15, 0.0);            // 深咖啡色邊框

    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x+w, y);
        glVertex2f(x+w, y-h);
        glVertex2f(x, y-h);
    glEnd();
    glLineWidth(1.0);
}

void display() {
    // 深灰色視窗背景
    glClearColor(0.15f, 0.18f, 0.20f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 繪製頂部狀態列背景
    glColor3f(0.1f, 0.1f, 0.15f);
    glBegin(GL_QUADS);
        glVertex2f(-1.0, 1.0); glVertex2f(1.0, 1.0);
        glVertex2f(1.0, 0.75); glVertex2f(-1.0, 0.75);
    glEnd();

    // 顯示狀態列文字
    char info[100];
    void* uiFont = GLUT_BITMAP_HELVETICA_18;
    if (state == 0) {
        drawText(-0.55, 0.85, "Welcome to Dark Chess! Press '1' (Player First) or '2' (AI First)", 1, 1, 1, uiFont);
    } else if (state == 1) {
        sprintf(info, "Turn: %s   |   Player (%s): %d/10   |   Comp: %d/10", 
                turn == 0 ? "Player" : "Computer",
                playerColor == 0 ? "?" : (playerColor == 1 ? "Red" : "Black"),
                playerMoves, compMoves);
        drawText(-0.7, 0.85, info, 1, 0.9, 0.6, uiFont);
    } else {
        drawText(-0.25, 0.85, "--- GAME OVER ---", 1, 0.3, 0.3, uiFont);
    }

    // 繪製木紋棋盤底色
    glColor3f(0.82f, 0.65f, 0.40f);
    glBegin(GL_QUADS);
        glVertex2f(-0.82, 0.52); glVertex2f(0.82, 0.52);
        glVertex2f(0.82, -0.72); glVertex2f(-0.82, -0.72);
    glEnd();

    // 繪製棋盤格子線
    glColor3f(0.5f, 0.3f, 0.1f);
    glLineWidth(2.0);
    glBegin(GL_LINES);
    for(int i=0; i<=ROWS; i++) {
        glVertex2f(-0.8, 0.5 - i*0.3); glVertex2f(0.8, 0.5 - i*0.3);
    }
    for(int j=0; j<=COLS; j++) {
        glVertex2f(-0.8 + j*0.2, 0.5); glVertex2f(-0.8 + j*0.2, -0.7);
    }
    glEnd();

    // 繪製棋子
    for (int i=0; i<ROWS; i++) {
        for (int j=0; j<COLS; j++) {
            if (board[i][j] == 0) continue;

            // 每格的寬高與邊界留白
            float cellW = 0.2f, cellH = 0.3f;
            float px = -0.8f + j * cellW + 0.02f;
            float py = 0.5f - i * cellH - 0.03f;
            float pw = cellW - 0.04f;
            float ph = cellH - 0.06f;

            int selected = (i == selR && j == selC);

            if (revealed[i][j]) {
                // 翻開：象牙白正面
                drawPieceRect(px, py, pw, ph, 0.95f, 0.92f, 0.85f, selected);
                
                // 繪製文字 (置中)
                int isRed = (board[i][j] <= 7);
                float tr = isRed ? 0.85f : 0.1f;
                float tg = isRed ? 0.15f : 0.1f;
                float tb = isRed ? 0.15f : 0.1f;
                drawText(px + pw/2.0f - 0.035f, py - ph/2.0f - 0.03f, names[board[i][j]], tr, tg, tb, GLUT_BITMAP_TIMES_ROMAN_24);
            } else {
                // 未翻開：深木色背面
                drawPieceRect(px, py, pw, ph, 0.70f, 0.45f, 0.25f, selected);
            }
        }
    }
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (state == 0) {
        if (key == '1') { state = 1; turn = 0; glutPostRedisplay(); }
        else if (key == '2') { 
            state = 1; turn = 1; 
            glutPostRedisplay();
            glutTimerFunc(500, computerTurnTimer, 0); 
        }
    }
}

void mouse(int button, int state_btn, int x, int y) {
    if (state != 1 || turn != 0 || playerMoves >= 10) return;

    if (button == GLUT_LEFT_BUTTON && state_btn == GLUT_DOWN) {
        int win_w = glutGet(GLUT_WINDOW_WIDTH);
        int win_h = glutGet(GLUT_WINDOW_HEIGHT);
        
        float normX = (float)x / win_w * 2.0 - 1.0;
        float normY = 1.0 - (float)y / win_h * 2.0;

        int c = -1, r = -1;
        // 將滑鼠座標精準映射到新的 UI 尺寸
        if (normX >= -0.8 && normX <= 0.8) c = (normX - (-0.8)) / 0.2;
        if (normY <= 0.5 && normY >= -0.7) r = (0.5 - normY) / 0.3;

        if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
            if (!revealed[r][c] && board[r][c] != 0) {
                revealed[r][c] = 1;
                if (playerColor == 0) {
                    playerColor = getPieceColor(board[r][c]);
                    compColor = (playerColor == 1) ? 2 : 1;
                }
                playerMoves++;
                turn = 1;
                selR = -1; selC = -1;
                checkGameOver();
                glutPostRedisplay();
                glutTimerFunc(500, computerTurnTimer, 0);
            } 
            else if (revealed[r][c] && board[r][c] != 0 && isPlayerPiece(board[r][c])) {
                selR = r; selC = c;
                glutPostRedisplay();
            } 
            else if (selR != -1 && selC != -1) {
                if (isValidMove(selR, selC, r, c)) {
                    board[r][c] = board[selR][selC]; revealed[r][c] = 1;
                    board[selR][selC] = 0; revealed[selR][selC] = 0;
                    playerMoves++;
                    turn = 1;
                    selR = -1; selC = -1;
                    checkGameOver();
                    glutPostRedisplay();
                    glutTimerFunc(500, computerTurnTimer, 0);
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    initBoard();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 500); // 稍微加高視窗比例讓棋盤更漂亮
    glutCreateWindow("Mac Dark Chess - Pro UI Version");
    glOrtho(-1, 1, -1, 1, -1, 1);
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}