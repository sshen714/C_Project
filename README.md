# ♟️ 象棋暗棋遊戲 (團體作業一) - C 語言實作

本專案使用 C 語言與 WinBGIm 圖形庫開發，實現了經典的「暗棋」隨機洗牌、翻牌互動以及簡易電腦對弈功能。

## 📁 專案檔案架構 (File Structure)
* `main.c`: 遊戲核心邏輯（含洗牌、繪圖、座標計算與滑鼠互動）。
* `C_PROJECT.cbp`: Code::Blocks 專案設定檔。
* `Group_Project_01_Demo.mp4`: 遊戲操作實作演示影片。
* `include/` & `lib/`: 必要的 WinBGIm 圖形庫標頭檔與靜態庫。

## 🚀 功能特點 (Features)
1.  **棋盤隨機初始化**：自動產生 4x8 棋盤並將 32 顆棋子隨機洗牌蓋牌。
2.  **滑鼠互動翻牌**：偵測玩家滑鼠左鍵點擊，即時翻開棋子並顯示。
3.  **簡易電腦對弈**：玩家翻牌後，電腦會自動在剩餘蓋牌區域隨機翻開一顆棋子。
4.  **UI 渲染優化**：採用 `TRIPLEX_FONT` 向量字型與動態居中演算法，確保顯示清晰。

## 🎮 棋子顯示說明 (Piece Legend)
為了確保在不同系統環境下文字渲染的相容性，避免中文編碼（Big5/UTF-8）導致的亂碼問題，本專案採用 **羅馬拼音 (Pinyin)** 標示棋子：

| 陣營 | 拼音名稱 (Pinyin) | 對應中文 |
| :--- | :--- | :--- |
| **黑方 (Black)** | Jiang, Shi, Xiang, Ju, Ma, Pao, Zu | 將、士、象、車、馬、包、卒 |
| **紅方 (Red)** | Shuai, Shi, Xiang, Che, Ma, Pao, Bing | 帥、仕、相、俥、傌、炮、兵 |

## 🎥 實作演示 (Demo Video)
請點擊下方連結查看專案執行畫面：
**[Group_Project_01_Demo.mp4](./Group_Project_01_Demo.mp4)**

## 🛠️ 如何編譯與執行 (Build & Run)
1.  使用 **Code::Blocks (建議版本：EDU-Portable)** 開啟 `C_PROJECT.cbp`。
2.  確認編譯器設定為 **GNU GCC Compiler**。
3.  確認連結器參數 (Linker settings) 已包含：
    `-lbgi -lgdi32 -lcomdlg32 -luuid -loleaut32 -lole32`
4.  按下 `F9` 進行編譯並執行。

---
*本專案為 2026 年 C 語言程式設計課程團體作業成果。*