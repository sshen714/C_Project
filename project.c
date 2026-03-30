#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROWS 4
#define COLS 8
int board[ROWS][COLS];
int revealed[ROWS][COLS];
const char* names[] = {"?", "K", "G", "E", "R", "H", "C", "S", "k", "g", "e", "r", "h", "c", "s"};

// 電腦翻牌的計時器回呼函式
void computerTurnTimer(int value) {
    int count = 0;
    // 檢查是否還有棋子可以翻
    for(int i=0; i<ROWS; i++) 
        for(int j=0; j<COLS; j++) if(!revealed[i][j]) count++;

    if (count > 0) {
        int cr, cc;
        do {
            cr = rand() % ROWS;
            cc = rand() % COLS;
        } while (revealed[cr][cc]);
        
        revealed[cr][cc] = 1;
        printf("電腦翻開了 [%d, %d]\n", cr, cc);
        glutPostRedisplay(); // 更新畫面
    }
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

void drawText(float x, float y, const char* s, int red) {
    if (red) glColor3f(1, 0, 0); else glColor3f(1, 1, 1);
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *s++);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (int i=0; i<ROWS; i++) {
        for (int j=0; j<COLS; j++) {
            float x = -0.9 + j * 0.225;
            float y = 0.7 - i * 0.45;
            
            // 畫格子
            glBegin(GL_LINE_LOOP);
                glColor3f(1, 1, 1);
                glVertex2f(x, y); glVertex2f(x+0.2, y);
                glVertex2f(x+0.2, y-0.3); glVertex2f(x, y-0.3);
            glEnd();
            
            if (revealed[i][j]) {
                drawText(x+0.05, y-0.2, names[board[i][j]], board[i][j]<=7);
            } else {
                glColor3f(0.5, 0.3, 0.1);
                glBegin(GL_QUADS);
                    glVertex2f(x+0.02, y-0.02); glVertex2f(x+0.18, y-0.02);
                    glVertex2f(x+0.18, y-0.28); glVertex2f(x+0.02, y-0.28);
                glEnd();
            }
        }
    }
    glutSwapBuffers();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int win_w = glutGet(GLUT_WINDOW_WIDTH);
        int win_h = glutGet(GLUT_WINDOW_HEIGHT);
        int c = x / (win_w / COLS);
        int r = y / (win_h / ROWS);
        
        if (r < ROWS && c < COLS && !revealed[r][c]) {
            revealed[r][c] = 1; // 玩家立即翻牌
            glutPostRedisplay(); // 先畫出玩家翻的棋子
            
            // 預約電腦在 500 毫秒 (0.5秒) 後翻牌
            glutTimerFunc(500, computerTurnTimer, 0);
        }
    }
}

int main(int argc, char** argv) {
    initBoard();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 400);
    glutCreateWindow("Mac Dark Chess - GLUT Delay Version");
    glOrtho(-1, 1, -1, 1, -1, 1);
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}