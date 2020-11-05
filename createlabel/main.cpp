#define NOMINMAX
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include<ctime>
#include "opencv2/opencv.hpp"
#include"audio.h"
#include "librosa.h"



#pragma  pack (push,1)
struct program_t {
	int id;
	int block_list[16];
};
struct audio_path_t {
	const char* path;
	int month;
	int day;
	program_t pro_list[16];
};
#pragma pack(pop)

static audio_path_t audio_path[] = { {"mp3/1022", 10,22,
									   { //{10,{0,400,1019,1276,1989,2275,2779, 3016,3466,-1}},
										 //{12,{0,739, 1226, 1458, 1890, 2344, 3015, 3249, 3452,-1}},
										 //{15,{100,1029, 1530, 1777, 2204, 2397, 2962, 3186, 3469,-1}},
										 //{21,{0,159, 794, 1085, 1530, 1829, 2860, 3110,3398,-1}},
										 //{13,{0,703,1538, 1801, 2269, 2564, 2831,-1}},
										 //{18,{0,777,1069, 1369, 1593, 1920, 2685, 2969,3361,-1}},
										 //{22,{0,435,1195, 1352, 2034, 2223, 2663, 2852, 3275,-1}},
										   {11,{0,621, 803, 976, 1220, 1254, 2141,2460,2667,2944,3295,-1}},
										   {17,{0,635, 1178, 1386, 1971, 2175, 2613, 2803, 3456,-1}},
										   {19,{0,662,940,1172,1669,1871,2247,2534,2877,3037,3601,-1}},
										 {-1,{-1}}
									   }
                                      },

									 {"mp3/1029", 10,29,
									   { //{10,{0,262, 877, 1141, 1728, 2040, 2834, 3061,3450,-1}},
										 //{12,{0,629, 1224, 1436, 1821, 2282, 2852, 3050, 3370,-1}},
										 //{15,{100,953, 1360, 1595, 2097, 2257,2709, 2905, 3409,-1}},
										 //{21,{0,97,743, 914, 1771, 1942, 2977, 3138, 3437,-1}},
										 //{13,{0,600,1474,1735, 2199, 2440, 2746,-1}},
										 {11,{0,559,742, 916,1153, 1188, 2071, 2385,2589, 2896,3281,-1}},
										 {-1,{-1}}
									   }
									  },
	                                 
	                                 {"mp3/1105", 11,5,
									   { //{11,{0,346,972,1244,2059,2388,2854,3248,3440,-1}},
										 //{13,{0,654,1265, 1506, 2055, 2521, 2979,-1}},
										 //{16,{0,812, 1319, 1558, 1950, 2142, 2577, 2792, 3275,-1}},
										 //{22,{0,76, 714, 886, 1506, 1772, 2972,3259, 3395,-1}},
			                             //{19,{0,638,995,1307,1742, 2049, 2677,2966, 3374,-1}},
										 {18,{0,693,1062, 1279,1761,1968,2508,2725,3430,-1}},
										 {-1,{-1}}
									   }
									  },

									 {"mp3/1112", 11,12,
									   { //{10,{0,312, 941,1217,1833, 2139, 2698, 3046, 3468,-1}},
										 //{12,{0,631, 1345, 1567, 1915, 2370, 2783, 3018,3275,-1}},
										 //{15,{0,945, 1328, 1564, 2080, 2303, 2778, 3004, 3490,-1}},
										 //{21,{0,97, 750, 1014, 1587, 1898, 2915, 3152, 3388,-1}},
			                             //{22,{0,330,1115, 1303, 2094, 2312, 2826, 3012,3373,-1}},
	                                     {19,{0,599,992,1228,1667,1882, 2186,2473, 2623, 2956,3544,-1}},
										 {-1,{-1}}
									   }
									  }
	                                 /*,

									 {"mp3/1109", 11,9,
									   {    {7,{0,312, 941,1217,1833, 2139, 2698, 3046, 3468,-1}},
										    {9,{0,631, 1345, 1567, 1915, 2370, 2783, 3018,3275,-1}},
										    {12,{0,945, 1328, 1564, 2080, 2303, 2778, 3004, 3490,-1}},
										    {15,{0,97, 750, 1014, 1587, 1898, 2915, 3152, 3388,-1}},
										    {16,{0,330,1115, 1303, 2094, 2312, 2826, 3012,3373,-1}},
                                            {-1,{-1}}
									   }
									  },
	                                  
	                                 {"mp3/1110", 11,10,
									   {    {8,{0,312, 941,1217,1833, 2139, 2698, 3046, 3468,-1}},
										    {12,{0,631, 1345, 1567, 1915, 2370, 2783, 3018,3275,-1}},
										    {14,{0,945, 1328, 1564, 2080, 2303, 2778, 3004, 3490,-1}},
										    {16,{0,97, 750, 1014, 1587, 1898, 2915, 3152, 3388,-1}},
										    {19,{0,330,1115, 1303, 2094, 2312, 2826, 3012,3373,-1}},
										    {-1,{-1}}
									   }
									  }*/									
                                   };




static int audio_path_len = sizeof(audio_path) / sizeof(audio_path_t);

static const char* wavdata_path = "wav_data_2";
static const char* meldata_path = "mel_data_2";
static int wav_num = 38611;

static const char* train_label = "train_label.txt";
static const char* val_label = "val_label.txt";
static const char* test_label = "test_label.txt";
static std::ofstream train_f;
static std::ofstream test_f;
static std::ofstream val_f;

static Librosa rosa;


void create_wav() {
	char mp3[128];
	char adv_file[128];
	char radio_file[128];
	Audio audio;

	for (int i = 0; i < audio_path_len; i++) {
		for (int j = 0; j < sizeof(audio_path[i].pro_list) / sizeof(program_t); j++) {

			if (audio_path[i].pro_list[j].id < 0) break;
			sprintf_s(mp3, "%s/1300_2019-%02d-%02d_%d.mp3",
				audio_path[i].path,
				audio_path[i].month,
				audio_path[i].day,
				audio_path[i].pro_list[j].id);
		
			if (!audio.mp3_to_wav(mp3, "temp1.wav")) continue;
			if (!audio.wav_stero2mono("temp1.wav", "temp.wav")) continue;

			wav_head_t head;
			std::vector<int8_t> wav;
			if (!audio.wav2vec("temp.wav", wav, head)) continue;

			std::vector<int8_t> adv_wav;
			std::vector<int8_t> radio_wav;
			int type = 0;

			for (int k = 1; k < sizeof(audio_path[i].pro_list[j].block_list) / sizeof(int); k++) {
				int start = audio_path[i].pro_list[j].block_list[k - 1] * head.byte_rate;
				int end = wav.size();
				if (audio_path[i].pro_list[j].block_list[k] >= 0)
					end = audio_path[i].pro_list[j].block_list[k] * head.byte_rate;

				if (type == 0)
					adv_wav.insert(adv_wav.end(), wav.begin() + start, wav.begin() + end);
				else
					radio_wav.insert(radio_wav.end(), wav.begin() + start, wav.begin() + end);
				type = 1 - type;
				if (audio_path[i].pro_list[j].block_list[k] < 0) break;
			}
			if (adv_wav.size() > 0) {
				sprintf_s(adv_file, "%s/adv_%d.wav", audio_path[i].path, audio_path[i].pro_list[j].id);
				audio.vec2wav(adv_wav.data(), adv_wav.size(), adv_file, head.bit_per_sample, head.sr, head.chn);
			}
			if (radio_wav.size() > 0) {
				sprintf_s(radio_file, "%s/radio_%d.wav", audio_path[i].path, audio_path[i].pro_list[j].id);
				audio.vec2wav(radio_wav.data(), radio_wav.size(), radio_file, head.bit_per_sample, head.sr, head.chn);
			}
		}
	}
}

void create_melimg(Librosa &rosa, std::vector<int8_t> wav, const char*  imgpath) {

	Matrix ans;
	if (!rosa.mel_spectrogram(wav, ans)) return;
	ans.transpos();
	/*for (int i = 0; i < ans.rows; i++) {
		for (int j = 0; j < ans.cols; j++) {
			printf("%5f,", ans.data[i * ans.cols + j]);
		}
		printf("\n");
	}*/
	//printf("ans %d  %d \n", ans.rows, ans.cols);
	cv::Mat img(ans.rows / 2, 160, CV_32FC(1), ans.data.data());
	cv::Mat resultImage;
	cv::normalize(img, resultImage, 0, 1, CV_MINMAX);
	cv::convertScaleAbs(resultImage, resultImage, 255);
	cv::imwrite(imgpath, resultImage);
}

void create_data(const char* wav_file, int step, int len, int label) {

	wav_head_t head;
	std::vector<int8_t> wav;
	Audio audio;
	char strwav[128];
	char strmel[128];
	char strlabel[128];

	if (!audio.wav2vec(wav_file, wav, head)) return;
	rosa.set_param(head.sr, head.bit_per_sample);
	int rs = step * head.byte_rate;
	int rl = len * head.byte_rate;
	for (int i = 0; i < wav.size() - rl; i += rs) {

		sprintf_s(strwav, "%s/%d.wav", wavdata_path, wav_num);
		sprintf_s(strlabel, "%d.wav", wav_num);
		sprintf_s(strmel, "%s/%d.jpg", meldata_path, wav_num);

		std::vector<int8_t> block_wav;
		block_wav.insert(block_wav.end(), wav.begin() + i, wav.begin() + i + rl);
		if (!audio.vec2wav(block_wav.data(), block_wav.size(), strwav, head.bit_per_sample, head.sr, head.chn))
			continue;
		create_melimg(rosa, block_wav, strmel);
		wav_num++;
		int rnd = rand() % 100;
		if (rnd < 85)
			train_f << strlabel << " " << label << std::endl;
		else if (rnd < 95)
			test_f << strlabel << " " << label << std::endl;
		else
			val_f << strlabel << " " << label << std::endl;
	}
}

int create_label(int step, int len) {

	train_f.open(train_label, std::ios::out | std::ios::app);
	if (!train_f.is_open()) {
		printf("open file %s error!\n", train_label);
		return 0;
	}

	test_f.open(test_label, std::ios::out | std::ios::app);
	if (!test_f.is_open()) {
		printf("open file %s error!\n", test_label);
		return 0;
	}

	val_f.open(val_label, std::ios::out | std::ios::app);
	if (!val_f.is_open()) {
		printf("open file %s error!\n", val_label);
		return 0;
	}
	char file[128];	
	for (int i = 0; i < audio_path_len; i++) {
		for (int j = 0; j < sizeof(audio_path[i].pro_list) / sizeof(program_t); j++) {
			
			if (audio_path[i].pro_list[j].id < 0) break;
			sprintf_s(file, "%s/adv_%d.wav", audio_path[i].path, audio_path[i].pro_list[j].id);
			//printf("%s\n", file);
			create_data(file, step, len, 0);
			sprintf_s(file, "%s/radio_%d.wav", audio_path[i].path, audio_path[i].pro_list[j].id);
			//printf("%s\n", file);
			create_data(file, step, len, 1);			
		}
	}
	train_f.close();
	test_f.close();
	val_f.close();
	return 1;
}

void getDataAndLable() {
	srand(time(NULL));
	create_wav();
	create_label(2, 4);
}

void testWav() {
	Audio audio;

	//audio.wav_to_mp3("4.wav", "4.mp3");
	//audio.mp3_to_wav("4.mp3", "5.wav");
	//audio.wav_down_sr("47737.wav", "43.wav", 43000);
	wav_head_t head;
	std::vector<int8_t> wav;
	audio.wav2vec("4.wav", wav, head);
	rosa.set_param(head.sr, head.bit_per_sample);
	create_melimg(rosa, wav, "t9_1.jpg");

	audio.adjust_tone_wav("4.wav", "9_tone.wav", 1000.0);
	audio.wav2vec("9_tone.wav", wav, head);
	rosa.set_param(head.sr, head.bit_per_sample);
	create_melimg(rosa, wav, "9_tone.jpg"); 
}


void test_fft() {

	Librosa rosa;
	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav1, wav2;

	if (!audio.wav2vec("4.wav", wav1, head)) return;
	Math_tool tool;
	std::vector<float> fdata1;
	tool.char2GTYPE(wav1, fdata1, head.bit_per_sample);
	Complex_Matrix md;
	int32_t n_fft = 1024;
	rosa.fft(fdata1, md, n_fft);
	std::vector<float> fdata2;
	rosa.ifft(md, fdata2, n_fft);
	tool.GTYPE2char(fdata2, wav2, head.bit_per_sample);

	if (!audio.vec2wav(wav2.data(),wav2.size(),"test2.wav",head.bit_per_sample, head.sr, head.chn)) return;
	return;
}


void test_stft() {

	Librosa rosa;
	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav1,wav2;

	if (!audio.wav2vec("D12_752.wav", wav1, head)) return;
	//if (!audio.wav2vec("47737.wav", wav2, head)) return;

	//printf("%d , %d \n", wav.size(), head.sr);


	//rosa.set_param(head.sr, head.bit_per_sample);
	//create_melimg(rosa, wav, "t9_4.jpg");
	//return;
	rosa.set_param(head.sr, head.bit_per_sample);

	std::vector<float> fdata1, fdata2;
	Math_tool tool;
	tool.char2GTYPE(wav1, fdata1, head.bit_per_sample);
	//tool.char2GTYPE(wav2, fdata2, head.bit_per_sample);

	Complex_Matrix md1, md2;
	rosa.stft(fdata1, md1);
	//rosa.stft(fdata2, md2);
	Matrix mag1;
	mag1 = md1.abs();
	//mag1.add(1000);

	md2 = md1.norm();
	rosa.compose_wav(mag1, md2, wav1);
	audio.vec2wav(wav1.data(), wav1.size(), "p2.wav", head.bit_per_sample, head.sr, head.chn);
	return;	
}

void test_mel() {

	Librosa rosa;
	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav1, wav2;

	Matrix ans;
	std::clock_t begin = clock();
	if (!audio.wav2vec("D12_752.wav", wav2, head)) return;
	rosa.set_param(head.sr, head.bit_per_sample, 0.0125, 0.05);
	//rosa.set_param(head.sr, head.bit_per_sample);
	rosa.mel_spectrogram(wav2, ans);
	printf("all time is %d\n", clock() - begin);

	//ans.transpos();
	//ans.show("log_mel:");

	begin = clock();
	rosa.inv_mel_spectrogram(ans, wav2, 1.5);
	printf("all time is %d\n", clock() - begin);
	audio.vec2wav(wav2.data(), wav2.size(), "p302.wav", head.bit_per_sample, head.sr, head.chn);
	//audio.adjust_tone_wav("ans.wav", "ans1.wav", 10);
	//ans.show("log_mel:");
}


int  main()
{
	//∂‡∫À‘ÀÀ„
	//Eigen::initParallel();
	//test_fft();
	//test_stft();
	//testWav();
	//test_mel();
	getDataAndLable();	
	return 1;
}



