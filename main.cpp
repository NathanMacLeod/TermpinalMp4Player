#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "olcPGEX_Sound.h"
#include "ConsoleEngine.h"
#include "cstdio"
#include <iostream>    
#include <fstream>
#include <stdio.h>
#include <chrono>
#include "Color.h"
#include <cstdlib>
#include <time.h>
#include <vector>

//std::string directory("D:/DiskDocuments/TerminalMp4Player");
//std::string directory_w("D:\\DiskDocuments\\TerminalMp4Player");
std::vector<std::string> video_list;
const int buffSize = 40096;
char outBuffer[buffSize];
int outIndex = 0;
char url[1024];

class Dimensions {
public:
	int width;
	int height;
	Dimensions(int width, float ratio) {
		this->width = width;
		this->height = width * ratio;
	}
};

float get_aspect_ratio(const char* name) {
	sprintf_s(url, "%s/codec_info.txt", name);
	std::ifstream codec_info(url);

	std::string line;
	std::getline(codec_info, line);
	std::getline(codec_info, line);
	strcpy_s(url, line.c_str());

	char* token;
	char* next;
	token = strtok_s(url, "x", &next);
	int width = atoi(token);
	token = strtok_s(NULL, "x", &next);
	int height = atoi(token);

	return height / (float)width;
}

void render_progress_bar(float percent, int width, bool done) {
	if (!done) {
		printf("[");
		int prog = percent * width;
		for (int i = 0; i < width; i++) {
			if (i <= prog) {
				printf("#");
			}
			else {
				printf("-");
			}
		}
		printf("] %d%% %\r", (int)(100 * percent));
	}
	else {
		printf("[");
		for (int i = 0; i < width; i++) {
			printf("#");
		}
		printf("] %%100\n");
	}
}

float get_fps(const char* name) {
	sprintf_s(url, "%s/codec_info.txt", name);
	std::ifstream codec_info(url);

	std::string line;
	std::getline(codec_info, line);
	strcpy_s(url, line.c_str());
	
	char* token;
	char* next;
	token = strtok_s(url, "/", &next);
	int num = atoi(token);
	token = strtok_s(NULL, "/", &next);
	int denom = atoi(token);

	return num / (float)denom;
}

int get_frame_count(const char* name) {
	sprintf_s(url, "%s/codec_info.txt", name);
	std::ifstream codec_info(url);

	std::string line;
	std::getline(codec_info, line);
	std::getline(codec_info, line);
	std::getline(codec_info, line);
	
	return atoi(line.c_str());
}

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

bool prerenderFrame(const char* name, std::ofstream& out, int outw, int outh, int curr, std::string& gstring, int scaleSize) {
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

void play_video(std::string name, bool use_pre_render, Dimensions dimensions=Dimensions(1, 1), int charS=0) {

	float fr = get_fps(name.c_str());

	olc::SOUND::InitialiseAudio();
	//printf("%s/%s/%s.wav\n", directory.c_str(), name.c_str(), name.c_str());
	//printf("%s/%s/%s_pre_render.txt\n", directory.c_str(), name.c_str(), name.c_str());

	if (use_pre_render) {
		sprintf_s(url, "%s/pre_render_info.txt", name.c_str());
		std::ifstream render_info(url);
		std::string line;

		std::getline(render_info, line);
		dimensions.width = atoi(line.c_str());
		std::getline(render_info, line);
		dimensions.height = atoi(line.c_str());
		std::getline(render_info, line);
		charS = atoi(line.c_str());
	}

	

	sprintf_s(url, "%s/%s.wav", name.c_str(), name.c_str());
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
	int buffSize = dimensions.width * dimensions.height;
	if (use_pre_render) {
		inBuff = new char[buffSize];
		sprintf_s(url, "%s/%s_pre_render.txt", name.c_str(), name.c_str());
		in = std::ifstream(url);
	}

	printf("%d, %d, %d\n", dimensions.width, dimensions.height, charS);
	ConsoleEngine engine(dimensions.width, dimensions.height, charS);
	olc::SOUND::PlaySample(music);

	if (use_pre_render) {
		readFrame(engine, inBuff, dimensions.width * dimensions.height, 1, in);
	}
	else {
		prepareFrame(name.c_str(), engine, dimensions.height, dimensions.width, curr, grayscale, scaleSize);
	}

	while (true) {
		t2 = std::chrono::system_clock::now();
		std::chrono::duration<float> timePassed = t2 - t1;
		t1 = t2;
		totalT += timePassed.count();

		int frameN = 1 + totalT * fr;
		if (curr != frameN) {


			engine.outputScreen();

			if (use_pre_render) {
				readFrame(engine, inBuff, buffSize, frameN - curr, in);
			}
			else {
				prepareFrame(name.c_str(), engine, dimensions.height, dimensions.width, curr + 1, grayscale, scaleSize);
			}

			curr = frameN;
		}
	}
}

void create_pre_render(std::string name, bool simple_grayscale, Dimensions dimensions, int charS) {
	int fr = 25;
	int frame_count = get_frame_count(name.c_str());

	std::string scale1(" .'^\",:;Il!i~+_-?][}{1)(|\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhoak*#MW&8%B@$"); //66
	std::string scale2(" .:-=+*#%@"); //10

	std::string grayscale;
	if (simple_grayscale) {
		grayscale = scale2;
	}
	else {
		grayscale = scale1;
	}
	int scaleSize = grayscale.size();

	sprintf_s(url, "%s/%s_pre_render.txt", name.c_str(), name.c_str());
	std::ofstream out(url);
	printf("Starting pre render...\n");

	int curr = 1;
	int perc = 100 * (curr - 1) / frame_count;
	while (prerenderFrame(name.c_str(), out, dimensions.width, dimensions.height, curr++, grayscale, scaleSize)) {
		int curr_perc = 100 * (curr - 1) / frame_count;
		if (curr_perc != perc) {
			perc = curr_perc;
			render_progress_bar((curr - 1) / (float)frame_count, 70, false);
		}
	}
	render_progress_bar(1, 70, true);

	flushBuffer(outIndex, out);
	out.close();
	printf("Done\n");

	sprintf_s(url, "%s/pre_render_info.txt", name.c_str());
	std::ofstream render_info(url);
	render_info << dimensions.width << std::endl << dimensions.height << std::endl << charS << std::endl;
	render_info.close();
}

bool file_exists(std::string file) {
	std::ifstream fstream(file);
	return fstream.is_open();
}

int main(int argc, char* argv[])
{

	sprintf_s(url, "videonames.txt");
	std::ifstream videonames(url);

	if (!videonames.is_open()) {
		std::ofstream create_file(url);
		create_file.close();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		videonames = std::ifstream(url);
	}
	if (!videonames.is_open()) {
		std::ofstream create_file(url);
		printf("Failed to find file %s\n", url);
		return 1;
	}
	else {
		std::string name;
		while (std::getline(videonames, name)) {
			video_list.push_back(name);
		}
	}
	videonames.close();


	enum State {PLAY, RENDER, DYNAMIC_RENDER, UNPACK, HELP, LIST, DEL_UNPACK, DEL, NONE};
	bool use_pre_render;
	std::vector<std::string> arguments;
	State selected = NONE;

	if (argc != 1) {
		char* arg = argv[1];
		if (strcmp(arg, "-h") == 0 || strcmp(arg, "-help") == 0) {
			selected = HELP;
		}
		else if (strcmp(arg, "-l") == 0 || strcmp(arg, "-list") == 0) {
			selected = LIST;
		}
		else if (strcmp(arg, "-p") == 0 || strcmp(arg, "-play") == 0) {
			selected = PLAY;
			if (argc <= 2) {
				printf("insufficient arguments for option -p. Need to include the name of the video. To view a list of all names added use the -l option\n");
				return 1;
			}
			else {
				arguments.push_back(std::string(argv[2]));
			}
		}
		else if (strcmp(arg, "-d") == 0 || strcmp(arg, "-dynamic_render") == 0) {
			selected = DYNAMIC_RENDER;
			if (argc <= 4) {
				printf("insufficient arguments for option -d. Need to include the name of the video, output width, and text size. Use -h for the full specification\n");
				return 1;
			}
			else {
				arguments.push_back(std::string(argv[2]));
				arguments.push_back(std::string(argv[3]));
				arguments.push_back(std::string(argv[4]));
			}
		}
		else if (strcmp(arg, "-u") == 0 || strcmp(arg, "-unpack") == 0) {
			selected = UNPACK;
			if (argc <= 3) {
				printf("insufficient arguments for option -u. Need to include the url of the mp4 file, and the desired name\n");
				return 1;
			}
			else {
				arguments.push_back(std::string(argv[2]));
				arguments.push_back(std::string(argv[3]));
			}
		}
		else if (strcmp(arg, "-r") == 0 || strcmp(arg, "-render_txt") == 0) {
			selected = RENDER;
			if (argc <= 4) {
				printf("insufficient arguments for option -r. Need to include the name of the video, output width, and text size. Use -h for the full specification\n");
				return 1;
			}
			else {
				arguments.push_back(std::string(argv[2]));
				arguments.push_back(std::string(argv[3]));
				arguments.push_back(std::string(argv[4]));
			}
		}
		else if (strcmp(arg, "-del_unpack") == 0) {
			selected = DEL_UNPACK;
			if (argc <= 2) {
				printf("insufficient arguments for option -del_unpack. Need to include the name of the video. Use -h for the full specification\n");
				return 1;
			}

			arguments.push_back(std::string(argv[2]));
		}
		else if (strcmp(arg, "-delete") == 0) {
			selected = DEL;
			if (argc <= 2) {
				printf("insufficient arguments for option -delete. Need to include the name of the video. Use -h for the full specification\n");
				return 1;
			}

			arguments.push_back(std::string(argv[2]));
		}
		else {
			printf("Unrecognized argument. Use -h or -help to get a list of supported actions\n");
		}
	}

	switch (selected) {
	case HELP:
	{
		printf("Supported arguments are:\n-list: List all video names that have been added\n-play [name]: Play txt render with for video saved by under name [name]\n");
		printf("-unpack [example_file.mp4] [name]: unpack a .mp4 file into .wav and .pngs of frames, save and internally store with name [name]\n-render_txt [name] [width] [text_size]: Create txt render file of video with name [name]\n");
		printf("-dynamic_render [name] [width] [text_size]: Render on the fly using unpacked .png files under name name\n-del_unpack [name]: deletes the unpacked .pngs to save space. You can still play a txt_render of the video if it has been created\n");
		printf("-delete [name] deletes entry of video under name\n");
		printf("\nIn order to play a .mp4 file, you must first unpack it and save it under a name using the -unpack flag. You can then either play the video dynamically with -dynamic_render, or create a txt render using -render.\n");
		printf("Text renders tend to have better performance, and are played with the -p or -play flag. They can be played even if the unpacked .png's deleted, in order to save space\n");
		break;
	}

	case LIST:
	{
		printf("      NAME      | REND | UNPAC  \n________________________________\n");
		for (int i = 0; i < video_list.size(); i++) {
			std::string name = video_list.at(i);
			sprintf_s(url, "%s/%s_frames/%s_frame1.png", name.c_str(), name.c_str(), name.c_str());
			bool unpac = file_exists(url);
			sprintf_s(url, "%s/%s_pre_render.txt", name.c_str(), name.c_str());
			bool rend = file_exists(url);
			printf("%-16s|   %s  |   %s  \n", name.c_str(), rend ? "x" : " ", unpac ? "x" : " ");
		}
		break;
	}

	case PLAY:
	{
		sprintf_s(url, "%s/%s_pre_render.txt", arguments[0].c_str(), arguments[0].c_str());
		bool pre_render_exists = file_exists(url);
		if (pre_render_exists) {
			play_video(arguments[0], true);
		}
		else {
			printf("Pre render under name %s not found. Use -l to get a list of stored names\n", arguments[0].c_str());
		}
		break;
	}

	case DYNAMIC_RENDER:
	{
		sprintf_s(url, "%s/%s_frames/%s_frame1.png", arguments[0].c_str(), arguments[0].c_str(), arguments[0].c_str());
		bool frames_exist = file_exists(url);
		if (!frames_exist) {
			printf("Unpacked frames not found under name %s\n", arguments[0].c_str());
			return 1;
		}
		int outw = atoi(arguments[1].c_str());
		int textS = atoi(arguments[2].c_str());
		play_video(arguments[0], false, Dimensions(outw, get_aspect_ratio(arguments[0].c_str())), textS);
		break;
	}

	case UNPACK:
	{
		std::string name = arguments[1];
		std::string mp4_url = arguments[0];

		//make directory to store all frames in

		sprintf_s(url, "mkdir %s\\%s_frames", name.c_str(), name.c_str());
		system(url);

		//add name to list of videos
		bool already_exists = false;
		for (int i = 0; i < video_list.size(); i++) {
			if (name == video_list.at(i)) {
				already_exists = true;
			}
		}

		if (!already_exists) {
			sprintf_s(url, "videonames.txt");
			std::ofstream videonames(url, std::ios::app);
			videonames << name << std::endl;
			videonames.close();
		}

		//get .wav from mp4
		sprintf_s(url, "ffmpeg -i %s %s\\%s.wav", mp4_url.c_str(), name.c_str(), name.c_str());
		system(url);

		//write fps to file codec_info.txt
		sprintf_s(url, "ffprobe -v error -select_streams v:0 -show_entries stream=avg_frame_rate -of default=noprint_wrappers=1:nokey=1 %s > %s\\codec_info.txt", mp4_url.c_str(), name.c_str());
		system(url);

		//write screen dimensions to file codec_info.txt
		sprintf_s(url, "ffprobe -v error -select_streams v:0 -show_entries stream=height,width -of csv=s=x:p=0 %s >> %s\\codec_info.txt", mp4_url.c_str(), name.c_str());
		system(url);

		//write total number of frames
		sprintf_s(url, "ffprobe -v error -select_streams v:0 -count_frames -show_entries stream=nb_read_frames -print_format default=nokey=1:noprint_wrappers=1 %s >> %s\\codec_info.txt", mp4_url.c_str(), name.c_str());
		system(url);

		//get all frames from mp4
		sprintf_s(url, "ffmpeg -i %s %s\\%s_frames\\%s_frame%%d.png", mp4_url.c_str(), name.c_str(), name.c_str(), name.c_str());
		system(url);
		break;
	}

	case RENDER:
	{
		std::string name = arguments[0];
		sprintf_s(url, "%s/%s_frames/%s_frame1.png", arguments[0].c_str(), arguments[0].c_str(), arguments[0].c_str());
		bool frames_exist = file_exists(url);
		if (!frames_exist) {
			printf("Unpacked frames not found under name %s\n", arguments[0].c_str());
			return 1;
		}
		int outw = atoi(arguments[1].c_str());
		int textS = atoi(arguments[2].c_str());

		create_pre_render(name, true, Dimensions(outw, get_aspect_ratio(arguments[0].c_str())), textS);
		break;
	}

	case DEL_UNPACK:
	{
		printf("Removing frames under %s, if they exist\n", arguments[0].c_str());
		sprintf_s(url, "rmdir /s /q %s\\%s_frames", arguments[0].c_str(), arguments[0].c_str());
		system(url);
		break;
	}

	case DEL:
	{
		std::string name = arguments[0];
		bool exists = false;
		for (int i = 0; i < video_list.size(); i++) {
			if (video_list.at(i) == name) {
				video_list.erase(video_list.begin() + i);
				exists = true;
				break;
			}
		}
		if (exists) {
			printf("Deleting entry for video %s\n", name.c_str());
			sprintf_s(url, "rmdir /s /q %s", arguments[0].c_str());
			system(url);

			sprintf_s(url, "videonames.txt");
			std::ofstream videonames(url);
			for (int i = 0; i < video_list.size(); i++) {
				videonames << video_list.at(i) << std::endl;
			}
		}
		else {
			printf("Video entry for %s not found\n", name.c_str());
			return 1;
		}
		break;
	}

	default:
		printf("Unrecognized command. Use -h or -help for a list of allowed options\n");
	}

}

