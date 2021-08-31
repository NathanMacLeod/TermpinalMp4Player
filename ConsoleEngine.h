#ifndef CONSOLEENGINE_H
#define CONSOLEENGINE_H

#include <Windows.h>
#include "olcPixelGameEngine.h"
#include <iostream>
#include <stdio.h>
#include "Color.h"
#include <vector>

class ConsoleEngine {
	CHAR_INFO* screen;
	HANDLE console;
	DWORD dwBytesWritten;
	SMALL_RECT rectangle;
	int sWidth;
	int sHeight;
	int piczelSize;
public:
	ConsoleEngine(int width, int height, int piczelSize) {

		console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
		sWidth = width;
		sHeight = height;
		rectangle = { 0, 0, (short)sWidth - 1, (short)sHeight - 1 };
		screen = new CHAR_INFO[width * height];
		

		COORD coord = { (short)sWidth - 1, (short)sHeight - 1};
		SetConsoleWindowInfo(console, TRUE, &rectangle);
		//SetConsoleScreenBufferSize(console, coord);

		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof(cfi);
		cfi.nFont = 0;
		cfi.dwFontSize.X = piczelSize;
		cfi.dwFontSize.Y = piczelSize;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;

		wcscpy_s(cfi.FaceName, L"Terminal");

		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(console, &info);

		if (!SetCurrentConsoleFontEx(console, false, &cfi))
			std::cout << "Failed to load font" << "\n";

		printf("Maximum size: %d, %d\n", info.dwMaximumWindowSize.X, info.dwMaximumWindowSize.Y);

		if (!SetConsoleScreenBufferSize(console, coord))
			std::cout << "Failed to set size" << "\n";
		

		SetConsoleActiveScreenBuffer(console);

	}
	~ConsoleEngine() {
		delete[] screen;
	}

	const short PICZEL = 0x2588;

	void outputScreen() {
		WriteConsoleOutput(console, screen, { (short)sWidth, (short)sHeight }, { 0,0 }, &rectangle);
	}

	int getPiczelIndex(int x, int y) {
		if (x < 0 || x >= sWidth || y < 0 || y >= sHeight)
			std::cout << "Illegal coordiantes entered" << std::endl;
		return x + y * sWidth;
	}

	void drawPiczel(int x, int y, short character, Color color) {
		drawPiczel(getPiczelIndex(x, y), character, color);
	}

	void drawPiczel(int index, short character, Color color) {
		screen[index].Char.UnicodeChar = character;
		screen[index].Attributes = color;
	}
	
};

#endif