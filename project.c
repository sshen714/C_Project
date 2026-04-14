#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// --- 自動偵測 Mac 或 Windows 環境 ---
#ifdef _WIN32
    #include <windows.h>
    #include <GL/glut.h>
#else
    #define GL_SILENCE_DEPRECATION
    #include <GLUT/glut.h>
#endif
// ------------------------------------

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define ROWS 4
#define COLS 8

int board[ROWS][COLS];
int revealed[ROWS][COLS];

// 中文字碼點：索引 1..14 對應棋子 ID (思源黑體支援所有字，還原正宗暗棋用字)
// 紅方(1-7): 帥 仕 相 俥 傌 炮 兵 ／ 黑方(8-14): 將 士 象 車 馬 砲 卒
static const int cnCodepoints[15] = {
    0,
    0x5E25, 0x4ED5, 0x76F8, 0x4FE5, 0x508C, 0x70AE, 0x5175, 
    0x5C07, 0x58EB, 0x8C61, 0x8ECA, 0x99AC, 0x7832, 0x5352  
};

typedef struct { GLuint tex; int w, h; } Glyph;
static Glyph glyphs[15];
static stbtt_fontinfo g_font;
static unsigned char* g_fontBuffer = NULL;
static int g_fontLoaded = 0;

static int loadFont(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    g_fontBuffer = (unsigned char*)malloc(sz);
    if (fread(g_fontBuffer, 1, sz, f) != (size_t)sz) { fclose(f); return 0; }
    fclose(f);
    int offset = stbtt_GetFontOffsetForIndex(g_fontBuffer, 0);
    if (!stbtt_InitFont(&g_font, g_fontBuffer, offset)) return 0;
    g_fontLoaded = 1;
    return 1;
}

static void bakeGlyph(int idx, int codepoint) {
    float scale = stbtt_ScaleForPixelHeight(&g_font, 96);
    int w = 0, h = 0, xoff = 0, yoff = 0;
    unsigned char* bm = stbtt_GetCodepointBitmap(&g_font, 0, scale, codepoint, &w, &h, &xoff, &yoff);
    if (!bm || w <= 0 || h <= 0) { glyphs[idx].tex = 0; return; }

    unsigned char* rgba = (unsigned char*)malloc(w * h * 4);
    for (int i = 0; i < w * h; i++) {
        rgba[i*4+0] = 255; rgba[i*4+1] = 255; rgba[i*4+2] = 255;
        rgba[i*4+3] = bm[i];
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glyphs[idx].tex = tex;
    glyphs[idx].w = w;
    glyphs[idx].h = h;

    free(rgba);
    stbtt_FreeBitmap(bm, NULL);
}

static void bakeAllGlyphs(void) {
    if (!g_fontLoaded) return;
    for (int i = 1; i <= 14; i++) bakeGlyph(i, cnCodepoints[i]);
}

static void drawGlyphCentered(float cx, float cy, float maxSize, int pieceId, int isRed) {
    if (pieceId < 1 || pieceId > 14) return;
    Glyph* g = &glyphs[pieceId];
    if (g->tex == 0) return;

    float aspect = (float)g->w / (float)g->h;
    float h = maxSize;
    float w = maxSize * aspect;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, g->tex);

    if (isRed) glColor4f(0.85f, 0.15f, 0.15f, 1.0f);
    else       glColor4f(0.10f, 0.10f, 0.10f, 1.0f);

    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2f(cx - w/2, cy + h/2);
        glTexCoord2f(1,0); glVertex2f(cx + w/2, cy + h/2);
        glTexCoord2f(1,1); glVertex2f(cx + w/2, cy - h/2);
        glTexCoord2f(0,1); glVertex2f(cx - w/2, cy - h/2);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

int state = 0; 
int turn = 0;  
int playerColor = 0; 
int compColor = 0;   
int playerMoves = 0;
int compMoves = 0;
int selR = -1, selC = -1;

int getPieceColor(int p) {
    if (p == 0) return 0;
    return (p <= 7) ? 1 : 2;
}

int isPlayerPiece(int p) { return getPieceColor(p) == playerColor; }
int isCompPiece(int p) { return getPieceColor(p) == compColor; }

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

void computerTurnTimer(int value) {
    if (state != 1 || turn != 1 || compMoves >= 10) return;
    int moved = 0;
    
    // 1. 避險
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

    // 2. 吃子
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
    turn = 0; 
    checkGameOver();
    glutPostRedisplay();
}

void initBoard() {
    int all[32] = {1,2,2,3,3,4,4,5,5,6,6,7,7,7,7,7, 8,9,9,10,10,11,11,12,12,13,13,14,14,14,14,14};
    srand((unsigned int)time(NULL));
    for (int i=0; i<32; i++) {
        int r = rand()%32;
        int t = all[i]; all[i] = all[r]; all[r] = t;
    }
    int k=0;
    for (int i=0; i<ROWS; i++) 
        for (int j=0; j<COLS; j++) { board[i][j]=all[k++]; revealed[i][j]=0; }
}

void drawText(float x, float y, const char* s, float r, float g, float b, void* font) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(font, *s++);
}

// 繪製圓形棋子
void drawPieceCircle(float cx, float cy, float radius, float r, float g, float b, int isSelected) {
    float aspect = 500.0f / 800.0f; // 修正視窗長寬比造成的橢圓變形
    int segments = 36;
    
    // 內部填色
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    for (int i = 0; i < segments; i++) {
        float theta = 2.0f * 3.1415926f * (float)i / (float)segments;
        glVertex2f(cx + radius * aspect * cos(theta), cy + radius * sin(theta));
    }
    glEnd();

    // 邊框與高亮
    glLineWidth(isSelected ? 5.0 : 2.5);
    if (isSelected) glColor3f(1.0f, 0.85f, 0.0f); // 金黃色高亮
    else glColor3f(0.3f, 0.15f, 0.0f);            // 深咖啡色邊框

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float theta = 2.0f * 3.1415926f * (float)i / (float)segments;
        glVertex2f(cx + radius * aspect * cos(theta), cy + radius * sin(theta));
    }
    glEnd();
    glLineWidth(1.0);
}

void display() {
    glClearColor(0.15f, 0.18f, 0.20f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.1f, 0.1f, 0.15f);
    glBegin(GL_QUADS);
        glVertex2f(-1.0, 1.0); glVertex2f(1.0, 1.0);
        glVertex2f(1.0, 0.75); glVertex2f(-1.0, 0.75);
    glEnd();

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

    glColor3f(0.82f, 0.65f, 0.40f);
    glBegin(GL_QUADS);
        glVertex2f(-0.82, 0.52); glVertex2f(0.82, 0.52);
        glVertex2f(0.82, -0.72); glVertex2f(-0.82, -0.72);
    glEnd();

    glColor3f(0.5f, 0.3f, 0.1f);
    glLineWidth(2.0);
    glBegin(GL_LINES);
    for(int i=0; i<=ROWS; i++) { glVertex2f(-0.8, 0.5 - i*0.3); glVertex2f(0.8, 0.5 - i*0.3); }
    for(int j=0; j<=COLS; j++) { glVertex2f(-0.8 + j*0.2, 0.5); glVertex2f(-0.8 + j*0.2, -0.7); }
    glEnd();

    for (int i=0; i<ROWS; i++) {
        for (int j=0; j<COLS; j++) {
            if (board[i][j] == 0) continue;

            float cellW = 0.2f, cellH = 0.3f;
            float cx = -0.8f + j * cellW + cellW / 2.0f;
            float cy = 0.5f - i * cellH - cellH / 2.0f;
            float radius = 0.125f; 

            int selected = (i == selR && j == selC);

            if (revealed[i][j]) {
                drawPieceCircle(cx, cy, radius, 0.95f, 0.92f, 0.85f, selected); // 正面

                int isRed = (board[i][j] <= 7);
                float glyphSize = radius * 1.3f;
                
                // 完全繪製中文字，移除英文備用機制
                if (g_fontLoaded && glyphs[board[i][j]].tex != 0) {
                    drawGlyphCentered(cx, cy, glyphSize, board[i][j], isRed);
                }
            } else {
                drawPieceCircle(cx, cy, radius, 0.70f, 0.45f, 0.25f, selected); // 背面
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
        
        float normX = (float)x / win_w * 2.0f - 1.0f;
        float normY = 1.0f - (float)y / win_h * 2.0f;

        int c = -1, r = -1;
        if (normX >= -0.8f && normX <= 0.8f) c = (int)((normX - (-0.8f)) / 0.2f);
        if (normY <= 0.5f && normY >= -0.7f) r = (int)((0.5f - normY) / 0.3f);

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
    glutInitWindowSize(800, 500); 
    glutCreateWindow("Dark Chess - Noto Sans UI");
    glOrtho(-1, 1, -1, 1, -1, 1);

    // 【終極修正】這裡包含了你下載的檔名，以及各種可能的備用檔名
    // 只要你的 NotoSansTC-VariableFont_wght.ttf 有放在同個資料夾就保證萬無一失！
    const char* fontPaths[] = {
        "./NotoSansTC-VariableFont_wght.ttf",             // 👑 使用者下載的思源黑體 (第一優先)
        "./myfont.ttf",                                   // 本地自行改名的備用字型
        "/System/Library/Fonts/PingFang.ttc",             // Mac 蘋方體 (系統備用)
        "/System/Library/Fonts/Supplemental/Songti.ttc",  // Mac 宋體 (系統備用)
        "C:\\Windows\\Fonts\\msjh.ttc",                   // Win 微軟正黑體 (系統備用)
        "C:\\Windows\\Fonts\\simhei.ttf"                  // Win 黑體 (系統備用)
    };
    
    int loaded = 0;
    // 逐一尋找陣列中的字型，直到找到一個能成功載入的為止
    for (int i = 0; i < 6; i++) {
        if (loadFont(fontPaths[i])) {
            printf("成功載入字型: %s\n", fontPaths[i]);
            loaded = 1;
            break;
        }
    }

    if (!loaded) {
        fprintf(stderr, "Error: 無法載入任何中文字型！請確認字型檔案是否有放在與程式相同的目錄下。\n");
    } else {
        bakeAllGlyphs();
    }

    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}