# 113-2 計算機程式設計：暗棋小遊戲

這是一個使用 C 語言 + OpenGL/GLUT 編寫的暗棋小遊戲，支援人機對戰、中文棋子顯示。

## 專案功能
1. **棋盤初始化**：自動生成 4x8 棋盤，並將 32 顆棋子（紅黑雙方）隨機打亂放入。
2. **視覺化介面**：使用圖形視窗顯示棋盤，未翻開時顯示為棕色背板，翻開後顯示中文棋子字（帥仕相俥傌炮兵／將士象車馬砲卒）。
3. **玩家操作**：支援滑鼠點擊未翻開的棋子進行翻牌，點選己方棋子後再點相鄰空格可移動。
4. **電腦對手**：玩家動作後，電腦會在 0.5 秒延遲後自動行動（避險 → 吃子 → 隨機翻牌）。
5. **顏色區分**：紅方棋子以 **紅色中文字** 顯示；黑方棋子以 **黑色中文字** 顯示。
6. **遊戲結束**：雙方各下滿 10 步後結束。

## 檔案結構
- `project.c`：遊戲核心程式碼（OpenGL 繪圖 + 遊戲邏輯）。
- `stb_truetype.h`：單一 header 字型函式庫，用於渲染中文字。
- `README.md`：專案說明文件。

---

## Windows 執行方式（預設）

目前 `project.c` 以 Windows 為預設環境。

### 依賴
- **MSYS2 / MinGW-w64**（含 `gcc`）
- **freeglut**：`pacman -S mingw-w64-ucrt-x86_64-freeglut`
- 系統字型：`C:/Windows/Fonts/msjh.ttc`（微軟正黑體，Windows 內建）

### 編譯與執行
```bash
gcc project.c -o project.exe -lfreeglut -lopengl32 -lglu32 -lm
./project.exe
```

---

## macOS 執行方式

macOS 內建 GLUT 與 OpenGL 框架，但需要針對以下兩處做平台調整：

### 步驟 1：修改 `project.c` 的 header include
將：
```c
#include <GL/glut.h>
```
改為：
```c
#include <GLUT/glut.h>
```

### 步驟 2：修改字型路徑
將 `main()` 中的字型載入路徑：
```c
if (!loadFont("C:/Windows/Fonts/msjh.ttc")) {
```
改為 macOS 內建的中文字型，例如：
```c
if (!loadFont("/System/Library/Fonts/PingFang.ttc")) {
```
（其他可選：`/Library/Fonts/Songti.ttc`、`/System/Library/Fonts/STHeiti Medium.ttc`）

### 步驟 3：編譯指令
在終端機輸入：
```bash
gcc project.c -o project -framework GLUT -framework OpenGL
./project
```

> ⚠️ macOS Catalina 之後 GLUT 被標記為 deprecated，但仍可使用（程式內已加 `#define GL_SILENCE_DEPRECATION` 抑制警告）。

---

## 操作說明
- 遊戲啟動後按 **`1`** 玩家先手、**`2`** 電腦先手。
- 點擊背面棋子 → 翻開。
- 點擊己方已翻開棋子（金黃高亮） → 再點相鄰空格 → 移動。
- 依暗棋規則吃子（將>士>象>車>馬>炮>卒，但卒可吃將；炮須隔一子跳吃）。
- 雙方各下滿 10 步後顯示 `GAME OVER`。
