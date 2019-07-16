#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <queue>
#include <algorithm>
#include <vector>
#include <stack>
#include <string>

#include <windows.h>
#include "easyx.h"

using namespace std;

namespace Reversi
{
	const int WIDTH = 620;
	const int HEIGHT = 720;
	const int MAXN = 18;

	/* Coordinate struct */
	struct Cord {
		int x, y;
	};
	/* chess board state */
	struct State {
		int s[MAXN][MAXN], black, white, curColor;
	};

	/* search direction */
	const Cord nextPos[8] = {
		{ -1, 0 },
		{ 1, 0 },
		{ 0, -1 },
		{ 0, 1 },
		{ -1, -1 },
		{ -1, 1 },
		{ 1, -1 },
		{ 1, 1 }
	};

	Cord leftTop, leftBottom, rightTop, rightBottom;

	int gridWidth, curColor = 0, radius, cnt;
	int board[MAXN][MAXN];
	int blackCount = 2, whiteCount = 2, gaming = 1;
	
	/* AI options */
	bool aiMode = true, aiColor = 1;
	int maxCnt = 0;

	vector<Cord> todo;
	stack<State> statePool;

	/**
	* print debug info
	* @param string info
	* @return void
	*/
	void printInfo(string info) {
		clearrectangle(0, HEIGHT - 70, WIDTH, HEIGHT - 30);

		settextcolor(BLACK);
		settextstyle(30, 0, _T("微软雅黑"));
		RECT textPos = { 0, HEIGHT - 70, WIDTH, HEIGHT };
		drawtext((LPCTSTR)info.c_str(), &textPos, DT_CENTER | DT_SINGLELINE);
	}

	/**
	* make a alert message
	* @param string title
	* @param string content
	* @return void
	*/
	void alert(string title, string content) {
		::MessageBox(NULL, ((LPCTSTR)content.c_str()), ((LPCTSTR)title.c_str()), 1);
	}
	
	/**
	* print chess count info on top-right corner
	* @return void
	*/
	void printChessCount() {
		clearrectangle(WIDTH - 150, 10, WIDTH - 20, 80);
		settextcolor(BLACK);
		settextstyle(20, 0, _T("微软雅黑"));

		RECT blackPos = { WIDTH - 150, 10, WIDTH - 20, HEIGHT };
		RECT whitePos = { WIDTH - 150, 30, WIDTH - 20, HEIGHT };
		RECT curPos = { WIDTH - 150, 50, WIDTH - 20, HEIGHT };
		string bkCount = "黑棋数量：" + to_string(blackCount);
		string wtCount = "白棋数量：" + to_string(whiteCount);
		string curPlayer = "现在轮到";
		curPlayer += curColor ? "白棋" : "黑棋";

		drawtext((LPCTSTR)bkCount.c_str(), &blackPos, DT_CENTER | DT_SINGLELINE);
		drawtext((LPCTSTR)wtCount.c_str(), &whitePos, DT_CENTER | DT_SINGLELINE);
		drawtext((LPCTSTR)curPlayer.c_str(), &curPos, DT_CENTER | DT_SINGLELINE);

		setfillcolor(curColor ? WHITE : BLACK);
		fillcircle(WIDTH - 160, 40, radius);
	}

	/**
	* check if you can put a chess on current position, and do reversing operation
	* @param int x
	* @param int y
	* @param int color  0/1
	* @param Cord dir   direction of next position
	* @param int cur    current step count
	* @return int       if can't put, returns 0, else returns x + 2, x is the chess you can reverse.
	*/
	int checkPos(int x, int y, int color, Cord dir, int cur) {
		if (x < 0 || y < 0 || x > cnt || y > cnt)
			return 0;
		if (cur != 0 && board[x][y] == -1)
			return 0;
		if (cur != 0 && board[x][y] == color)
			return 1;
		int ans = checkPos(x + dir.x, y + dir.y, color, dir, cur + 1);
		if (ans > 0) {
			board[x][y] = color;
			setfillcolor(color ? WHITE : BLACK);
			Cord chessPos = { x * gridWidth + leftTop.x, y * gridWidth + leftTop.y };
			fillcircle(chessPos.x, chessPos.y, radius);
			return ans + 1;
		}
		else
			return 0;
	}

	/**
	* check if game can be contiuned
	* @param int cur    current player color
	* @return int
	*/
	int checkGameStatus(int cur) {
		// someone won
		if (blackCount == 0)
			return 1;
		if (whiteCount == 0)
			return 0;
		maxCnt = 0;
		todo.clear();
		for (int i = 0; i <= cnt; i++) {
			for (int j = 0; j <= cnt; j++) {
				if (board[i][j] == -1)
					for (int k = 0; k < 8; k++) {
						int nextX = i + nextPos[k].x, nextY = j + nextPos[k].y;
						if (nextX < 0 || nextY < 0 || nextX > cnt || nextY > cnt)
							continue;
						if (board[nextX][nextY] == -1 || board[nextX][nextY] == cur)
							continue;
						int curCnt = 0, flag = 0;
						while (nextX >= 0 && nextX <= cnt && nextY >= 0 && nextY <= cnt) {
							curCnt++;
							if (board[nextX][nextY] == -1)
								break;
							if (board[nextX][nextY] == cur) {
								flag = 1;
								break;
							}
							curCnt++;
							nextX += nextPos[k].x, nextY += nextPos[k].y;
						}
						if (!flag)
							continue;
						if (!aiMode || cur != (int)aiColor)
							return -1;

						if (curCnt > maxCnt) {
							maxCnt = curCnt;
							todo.clear();
							todo.push_back({ i, j });
						} else if (curCnt == maxCnt && curCnt != 0)
							todo.push_back({ i, j });
					} // for k
			} // for j
		} // for i

		if (maxCnt)
			return -1;	// can be continued
		return 2;		// can't be continued
	}

	void AITurn();

	/**
	* draw a chess on board
	* @param int x
	* @param int y
	* @param int col
	* @return void
	*/
	void drawChess(int x, int y, int col) {
		setfillcolor(col ? WHITE : BLACK);
		Cord chessPos = { x * gridWidth + leftTop.x, y * gridWidth + leftTop.y };
		fillcircle(chessPos.x, chessPos.y, radius);
	}

	/**
	* save current chessboard state
	* @return
	*/
	void pushState() {
		State p;
		p.black = blackCount, p.white = whiteCount, p.curColor = curColor;
		
		for (int i = 0; i <= cnt; i++)
			for (int j = 0; j <= cnt; j++)
				p.s[i][j] = board[i][j];

		statePool.push(p);
	}

	void drawChessboard(int x, int y, int n, int width, COLORREF lineColor = BLACK);

	/**
	* recover the board from a certain state
	* @param State p         target state 
	* @return void
	*/
	void recoverFromState(State p) {
		for (int i = 0; i <= cnt; i++)
			for (int j = 0; j <= cnt; j++)
				board[i][j] = p.s[i][j];
		blackCount = p.black, whiteCount = p.white, curColor = p.curColor;

		drawChessboard((WIDTH - cnt * gridWidth) / 2, (HEIGHT - cnt * gridWidth) / 2, cnt, gridWidth);
		for (int i = 0; i <= cnt; i++)
			for (int j = 0; j <= cnt; j++)
				if (board[i][j] != -1)
					drawChess(i, j, board[i][j]);

		printChessCount();
	}

	/**
	* make an alert and end a game
	* @return void
	*/
	void endGame(int res) {
		gaming = 0;
		int tmp = 0;

		if (res == 0) {
			printInfo("黑棋胜利");
			tmp = ::MessageBox(NULL, TEXT("黑棋胜利!\n你想重新开始吗？"), TEXT("游戏结束"), 1);
		}
		else {
			printInfo("白棋胜利");
			tmp = ::MessageBox(NULL, TEXT("白棋胜利!你想重新开始吗？\n"), TEXT("游戏结束"), 1);
		}

		if (tmp == 1) {
			while (statePool.size() > 1)
				statePool.pop();
			State fin = statePool.top();
			recoverFromState(fin);
		}
	}

	/**
	* put a chess on (x, y)
	* @param int x
	* @param int y
	* @return void
	*/
	void putChess(int x, int y) {
		int curStatus = checkGameStatus(curColor);

		if (curStatus == 0 || curStatus == 1) {
			endGame(curStatus);
		}
		else if (curStatus == 2) {
			if (checkGameStatus(!curColor) == 2) { 
				endGame(blackCount < whiteCount);
				return;
			}
			printInfo("您没有棋可以下了：（");
			curColor ^= 1;
			AITurn();

			
			printChessCount();
			return;
		}
		
		if (board[x][y] != -1) {
			printInfo("你不能将棋子放在这里，因为此位置已经有了一个棋子。");
			return;
		}

		int flag = 0, revCnt = 0;

		// save current state
		pushState();

		for (int i = 0; i < 8; i++) {
			int nextX = x + nextPos[i].x,
				nextY = y + nextPos[i].y;

			if (nextX < 0 || nextY < 0 || nextX > cnt || nextY > cnt)
				continue;
			if (board[nextX][nextY] != curColor && board[nextX][nextY] != -1) {
				int tmp = checkPos(x, y, curColor, nextPos[i], 0);
				if (tmp)
					flag = 1, revCnt += (tmp == 0 ? 0 : tmp - 2);
			}
		}

		if (!flag) {
			printInfo("你不能将棋子放在这里，因为你无法翻转任何棋子。");
			statePool.pop();			// delete repeat state
			return;
		}

		blackCount += curColor ? -revCnt : revCnt + 1;
		whiteCount += curColor ? revCnt + 1 : -revCnt;
		board[x][y] = curColor;

		drawChess(x, y, curColor);
		
		curColor ^= 1;
		printChessCount();
		printInfo("");

		if (aiMode && curColor == (int)aiColor) {
			AITurn();
		}
	}

	/**
	* now it is AI's turn.
	* @return void
	*/
	void AITurn() {
		if (aiMode && curColor == (int)aiColor) {
			int curStatus = checkGameStatus(curColor),
				size = todo.size();
			if (curStatus == 0 || curStatus == 1) {
				endGame(curStatus);
			}
			if (size == 0) {
				printInfo("您没有棋可以下了：（");
				curColor ^= 1;
				printChessCount();
				return;
			}
			int sel = rand() % size;
			Sleep(500);
			putChess(todo[sel].x, todo[sel].y);
		}
	}

	/**
	* draw a chessboard
	* @param int x
	* @param int y
	* @param int n				  count of grid
	* @param int width			  width of grid
	* @param COLORREF lineColor   fill color of line
	* @return void
	*/
	void drawChessboard(int x, int y, int n, int width, COLORREF lineColor) {
		clearrectangle(x - width / 2, y - width / 2, x + n * width + width / 2, y + n * width + width / 2);

		setlinecolor(lineColor);
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
				rectangle(x + j * width, y + i * width, x + (j + 1) * width, y + (i + 1) * width);
		leftTop = { x, y }, leftBottom = { x, y + n * width },
			rightTop = { x + n * width }, rightBottom = { x + n * width, y + n * width };
		radius = gridWidth / 2 - 4;

		int half = n / 2;
		
		board[half][half] = board[half + 1][half + 1] = 0;
		board[half + 1][half] = board[half][half + 1] = 1;
		setfillcolor(BLACK);
		fillcircle(leftTop.x + half * gridWidth, leftTop.y + half * gridWidth, radius);
		fillcircle(leftTop.x + (half + 1) * gridWidth, leftTop.y + (half + 1) * gridWidth, radius);
		setfillcolor(WHITE);
		fillcircle(leftTop.x + (half + 1) * gridWidth, leftTop.y + half * gridWidth, radius);
		fillcircle(leftTop.x + half * gridWidth, leftTop.y + (half + 1) * gridWidth, radius);

		printChessCount();
	}

	/**
	* print title and copyright info
	* @return void
	*/
	void printTitleInfo() {
		settextstyle(30, 0, _T("微软雅黑"));
		settextcolor(BLACK);
		RECT textPos = { 0, 25, Reversi::WIDTH, Reversi::HEIGHT };
		drawtext(_T("黑白棋（翻转棋）"), &textPos, DT_CENTER | DT_SINGLELINE);

		settextstyle(20, 0, _T("微软雅黑"));
		settextcolor(BLUE);
		RECT copyrightPos = { 0, HEIGHT - 30, Reversi::WIDTH, Reversi::HEIGHT };
		drawtext(_T("@Copyright 2019, Yume Maruyama / XMU C Language Practice Project"), &copyrightPos, DT_CENTER | DT_SINGLELINE);
		settextcolor(BLACK);
	}

	/**
	* draw buttons
	* @return void
	*/
	void drawButtons() {
		clearrectangle(0, 0, 200, 80);
		setlinecolor(BLACK);
		rectangle(50, 10, 160, 40);
		rectangle(50, 40, 160, 70);


		RECT AIModePos = { 60, 15, 150, 35 };
		RECT regretPos = { 60, 36, 150, 75 };
		drawtext(_T("开启/关闭 AI"), &AIModePos, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		drawtext(_T("悔棋"), &regretPos, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	}

	/**
	* drawing a window and initialize game
	* @param int n
	* @param int w
	* @param COLORREF background
	* @return void
	*/
	void initGame(int n, int w, COLORREF background = 0xFCE5B3) {
		initgraph(WIDTH, HEIGHT, /* SHOWCONSOLE */ 0);
		setbkcolor(background);
		cleardevice();
		
		memset(board, -1, sizeof board);
		printTitleInfo();
		drawButtons();
		cnt = n, gridWidth = w;
		drawChessboard((WIDTH - cnt * gridWidth) / 2, (HEIGHT - cnt * gridWidth) / 2, cnt, gridWidth);
		
		blackCount = whiteCount = 2;

		srand((int)time(NULL));
	}

	/**
	* @return void
	*/
	void toggleAIMode() {
		aiMode ^= 1;
		aiColor = curColor;
		printInfo(aiMode ? "开启 AI 模式" : "关闭 AI 模式");
		AITurn();
	}

	/**
	* handle mouse message
	* @return void
	*/
	void listenMouseEvent() {
		MOUSEMSG msg;
		while (1) {
			msg = GetMouseMsg();

			switch (msg.uMsg) {
				case WM_MOUSEMOVE:
					break;
				case WM_LBUTTONDOWN:
					if ( gaming
						&& msg.x >= leftTop.x - gridWidth / 2 && msg.x <= rightTop.x + gridWidth / 2
						&& msg.y >= leftTop.y - gridWidth / 2 && msg.y <= rightBottom.y + gridWidth / 2 ) {
						int horizonPos = (msg.x - leftTop.x + gridWidth / 2) / gridWidth,
							verticalPos = (msg.y - leftTop.y + gridWidth / 2) / gridWidth;
						putChess(horizonPos, verticalPos);
					}
					if (msg.x >= 50 && msg.x <= 160 && msg.y >= 10 && msg.y <= 40) {
						toggleAIMode();
					}
					if (msg.x >= 50 && msg.x <= 160 && msg.y >= 41 && msg.y <= 70) {
						if (statePool.size() < (unsigned int)(aiMode ? 2 : 1)) {
							printInfo("不能再悔棋了 :(");
						}
						else {
							State prev = statePool.top();
							statePool.pop();
							if (aiMode) {
								prev = statePool.top();
								statePool.pop();
							}
							recoverFromState(prev);
						}
					}
					if (msg.y >= HEIGHT - 30) {
						alert("关于程序",
"2019厦门大学暑期C语言实践课程设计项目，一个简单的黑白棋小游戏。\n\
支持 PVP 和 PVE，内置简单的 AI.\n\
图形库采用 EasyX, 基于 Visual Studio 2017 构建，仅限于 Windows 平台运行。\n\
程序在三天之内完成，难免存在一些 BUG，若您遇到了问题，请联系：kirainmoe@gmail.com。\n\n\
@Copyright 2019, Yume Maruyama"
						);
					}
					break;
			}
		}
	}
};

int main()
{
	Reversi::initGame(13, 40);
	Reversi::listenMouseEvent();
	return 0;
}