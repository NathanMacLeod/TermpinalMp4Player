#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "olcPGEX_Sound.h"
#include "ConsoleEngine.h"
#include "cstdio"
#include <iostream>    
#include <stdio.h>
#include <chrono>
#include "Color.h"
#include <cstdlib>
#include <time.h>

const int buffSize = 40096;
char outBuffer[buffSize];
int outIndex = 0;

/*class Example : public olc::PixelGameEngine
{
public:
	Example()
	{
		sAppName = "Example";
	}

public:

	bool OnUserCreate() override
	{
		olc::SOUND::InitialiseAudio();
		int music = olc::SOUND::LoadAudioSample("D:/DiskDocuments/VisualStudiosProjects/TerminalMp4Player/TerminalMp4Player/akari/akari.wav");
		olc::SOUND::PlaySample(music);
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		static bool first = true;
		static int fr = 24;
		static float t = 0;
		static int curr = 0;

		int frameN = 1 + t * fr;
		if (curr != frameN && frameN <= 6404) {
			curr = frameN;

			const char* name = "akari";
			static char url[256];
			sprintf_s(url, "%s/%s_frames/%s_frame%d.png", name, name, name, curr);
			olc::Sprite frame(url);

			for (int i = 0; i < frame.width; i++) {
				for (int j = 0; j < frame.height; j++) {
					olc::Pixel pixel = frame.GetPixel(i, j);
					float gscale = 0.3 * pixel.r + 0.59 * pixel.g + 0.11 * pixel.b;
					Draw(i, j, olc::Pixel(gscale, gscale, gscale));
				}
			}
		}

		if (!first) {
			t += fElapsedTime;
		}
		first = false;
		return true;
	}
};*/

float getGreyscale(olc::Sprite& source, int x, int y, int outW, int outH, Color* colorOut=nullptr) {
	float wR = (float) source.width / outW;
	float hR = (float)source.height / outH;

	float avg = 0;
	int x0 = x * wR;
	int y0 = y * hR;

	int p = 0;
	for (int i = 0; i < (int)wR; i++) {
		for (int j = 0; j < (int)hR; j++) {
			olc::Pixel pixel = source.GetPixel(x0 + i, y0 + j);
			avg += 0.3 * pixel.r + 0.59 * pixel.g + 0.11 * pixel.b;
			p++;
		}
	}

	return avg / p;
}

void flushBuffer(int n, std::ofstream& out) {
	out.write(outBuffer, n);
}

void writeChar(char c, std::ofstream& out) {
	outBuffer[outIndex] = c;
	outIndex++;

	if (outIndex == buffSize) {
		outIndex = 0;
		flushBuffer(buffSize, out);
	}
}

bool prerenderFrame(const char* name, std::ofstream& out, int outh, int outw, int curr, std::string& gstring, int scaleSize) {
	static char url[256];
	sprintf_s(url, "%s/%s_frames/%s_frame%d.png", name, name, name, curr);
	olc::Sprite frame(url);
	if (frame.width == 0) {
		return false; //file not loaded->last frame
	}

	int charsWritten = 0;
	for (int i = 0; i < outh; i++) {
		for (int j = 0; j < outw; j++) {
			float gscale = getGreyscale(frame, j, i, outw, outh);
			writeChar(gstring[(gscale - 1) * scaleSize / 255], out);
			charsWritten++;
		}
	}
	//printf("Wrote %d chars in a frame. Buff size is %d\n", charsWritten, outw * outh);
	return true;
}

bool prepareFrame(const char* name, ConsoleEngine& engine, int outh, int outw, int curr, std::string& gstring, int scaleSize) {
	static char url[256];
	sprintf_s(url, "%s/%s_frames/%s_frame%d.png", name, name, name, curr);
	olc::Sprite frame(url);
	if (frame.width == 0) {
		return false; //file not loaded->last frame
	}
	for (int i = 0; i < outw; i++) {
		for (int j = 0; j < outh; j++) {
			float gscale = getGreyscale(frame, i, j, outw, outh);
			engine.drawPiczel(engine.getPiczelIndex(i, j), gstring[(gscale - 1) * scaleSize / 255], white);
		}
	}
	return true;
}

bool readFrame(ConsoleEngine& engine, char* buff, int buffSize, int nFrames, std::ifstream& in) {
	while (nFrames > 0) {
		in.read(buff, buffSize);
		nFrames--;
	}

	if (in.eof()) 
		return false;

	for (int i = 0; i < buffSize; i++) {
		engine.drawPiczel(i, buff[i], white);
	}

	return true;
}

int main(int argc, char * argv[])
{

	enum State {PRE_RENDER_CREATE, USE_PRE_RENDER, DYNAMIC_RENDER};
	State state = USE_PRE_RENDER;
	const char* name = "";
	int fr = 25;
	int outw = 320 / 1.1;
	int outh = 180 / 1.1;
	int charS = 10;

	enum Vids { RICK, AKARI, APPLE, NIGHTMARE, RAT, NECRON };
	Vids chosen = AKARI;

	if (argc == 2) {
		switch (argv[1][0]) {
		case 'a':
			chosen = AKARI;
			break;
		case 'b':
			chosen = APPLE;
			break;
		case 'r':
			chosen = RICK;
			break;
		}
	}

	
	switch (chosen) {
	case RICK:
		name = "rickroll";
		fr = 25;
		break;
	case AKARI:
		name = "akari";
		fr = 24;
		break;
	case APPLE:
		name = "bad_apple";
		fr = 30;
		break;
	case NIGHTMARE:
		name = "nightmare";
		fr = 30;
	case RAT:
		name = "rat";
		fr = 30;
		break;
	case NECRON:
		name = "necron";
		fr = 30;
		break;
	}

	

	olc::SOUND::InitialiseAudio();
	char url[1024];
	sprintf_s(url, "D:/DiskDocuments/VisualStudiosProjects/TerminalMp4Player/TerminalMp4Player/%s/%s.wav", name, name);
	int music = olc::SOUND::LoadAudioSample(url);

	//std::cout << "Press enter to begin\n";
	//std::cin.get();

	//ConsoleEngine engine(outw, outh, charS);
	//std::this_thread::sleep_for(std::chrono::seconds(25));
	
	std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point t2;

	double totalT = 0;

	bool first = true;
	float t = 0;
	int curr = 1;

	std::string scale1(" .'^\",:;Il!i~+_-?][}{1)(|\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhoak*#MW&8%B@$"); //66
	std::string scale2(" .:-=+*#%@"); //10

	std::string grayscale = scale2;
	int scaleSize = grayscale.size();

	std::ifstream in;
	char* inBuff = nullptr;
	int buffSize = outw * outh;
	if (state == USE_PRE_RENDER) {
		inBuff = new char[buffSize];
		sprintf_s(url, "%s/%s_pre_render.txt", name, name);
		in = std::ifstream(url);
	}

	if (state != PRE_RENDER_CREATE) {
		ConsoleEngine engine(outw, outh, charS);
		olc::SOUND::PlaySample(music);

		if (state == USE_PRE_RENDER) {
			readFrame(engine, inBuff, outw * outh, 1, in);
		}
		else {
			prepareFrame(name, engine, outh, outw, curr, grayscale, scaleSize);
		}

		while (true) {
			t2 = std::chrono::system_clock::now();
			std::chrono::duration<float> timePassed = t2 - t1;
			t1 = t2;
			totalT += timePassed.count();

			int frameN = 1 + totalT * fr;
			if (curr != frameN) {


				engine.outputScreen();

				if (state == USE_PRE_RENDER) {
					readFrame(engine, inBuff, buffSize, frameN - curr, in);
				}
				else {
					prepareFrame(name, engine, outh, outw, curr + 1, grayscale, scaleSize);
				}

				curr = frameN;
			}
		}
	}
	else {
		sprintf_s(url, "%s/%s_pre_render.txt", name, name);
		std::ofstream out(url);
		printf("Starting pre render...\n");
		while (prerenderFrame(name, out, outh, outw, curr++, grayscale, scaleSize)) {
			if (curr % 128 == 0) {
				printf("Currently Finished: %d frames\n", curr);
			}
		}
		flushBuffer(outIndex, out);
		out.close();
		printf("Done\n");
	}

	return 0;
}

