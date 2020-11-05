//#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include<ctime>
#include "opencv2/opencv.hpp"
#include"audio.h"
#include "librosa.h"

#pragma  pack (push,1)
struct audio_path_t {
	const char* path;
	int month;
	int day;
};
#pragma pack(pop)

static audio_path_t audio_path[] = { {"mp3/1022", 10,22},
							  {"mp3/1029",10,29},
							  {"mp3/1105",11,5},
							  {"mp3/1112",11,12} };
static int audio_path_len = sizeof(audio_path) / sizeof(audio_path_t);


void test_audio() {
	Audio audio;
	//audio.mp3_to_wav("mp3/1300_JinRiHuaTi_615.mp3", "4.wav",-200,400);
	//audio.wav_to_mp3("1.wav", "2.mp3", 0, 300);
	//audio.wav_stero2mono("3.wav", "5.wav");
	//audio.wav_mono2stero("5.wav", "6.wav");
	//audio.compose_wav("2.wav", "3.wav", "7.wav");
	//audio.merge_wav("2.wav", "3.wav", "8.wav");
	audio.cut_wav("1.wav", "9.wav", 70, 175.5);
}


void create_wav() {
	char mp3[128];
	char wav[128];
	Audio audio;

	for (int i = 0; i < audio_path_len; i++) {
		for (int j = 9; j < 25; j++) {
			sprintf_s(mp3, "%s/1300_2019-%02d-%02d_%d.mp3", 
				audio_path[i].path, 
				audio_path[i].month,
				audio_path[i].day,
				j);

			sprintf_s(wav, "%s/%d.wav", audio_path[i].path,	j - 9);
			audio.mp3_to_wav(mp3, "temp.wav");
			audio.wav_stero2mono("temp.wav", wav);
		}
	}
}

int get_rand_wav(const char* dst, float step, float start, float end=0.0f) {
	
	Audio audio;
	char srcfile[128];
	
	sprintf_s(srcfile, "%s/%d.wav", audio_path[rand() % audio_path_len].path, rand() % 16);
	int pos = 0;
	if (start < -FLT_EPSILON) 
		pos = rand() % (int)(floor(fabs(start)) - step);		
	else 
		pos = rand() % (int)(floor(end - start - step));
	//printf("pos = %d, start = %f, end = %f\n", pos, start+pos, start + pos + step);
	return audio.cut_wav(srcfile, dst, start + pos, start + pos + step);
}

void create_melimg(Librosa &rosa, const char* wavpath, const char*  imgpath) {

	Audio  audio;
	wav_head_t head;
	std::vector<int8_t> wav;

	if (!audio.wav2vec(wavpath, wav, head)) return;
	rosa.set_param(head.sr, head.bit_per_sample);
	Matrix ans;
	rosa.log_mel(wav, ans);
	ans.transpos();
	//printf("ans %d  %d \n", ans.rows, ans.cols);
	cv::Mat img(ans.rows / 2, 160, CV_32FC(1), ans.data.data());	
	cv::Mat resultImage;
	cv::normalize(img, resultImage, 0, 1, CV_MINMAX);
	cv::convertScaleAbs(resultImage, resultImage, 255);
	cv::imwrite(imgpath, resultImage);
}

void create_data(Librosa &rosa, 
	             int total, 
	             int step, 
				 float start,
	             float end,
	             const char *wav_path, 
	             const char *mel_path,
				 int type,
				 int &num,
	             std::ofstream &train_f,
	             std::ofstream &test_f,
	             std::ofstream &val_f){
	char wav[128];
	char mel[128];
	char label[128];
	while (num < total) {
		sprintf_s(wav, "%s/%d.wav", wav_path, num);
		sprintf_s(label, "%d.wav", num);
		sprintf_s(mel, "%s/%d.jpg", mel_path, num);
		if (!get_rand_wav(wav, step, start, end)) continue;
		create_melimg(rosa, wav, mel);
		num++;
		int rnd = rand() % 100;
		if (rnd < 85)
			train_f << label << " " << type << std::endl;
		else if (rnd < 95)
			test_f << label << " " << type << std::endl;
		else
			val_f << label << " " << type << std::endl;
	}
}

int create_label(int total) {

	const char* train_label = "train_label.txt";
	const char* val_label = "val_label.txt";
	const char* test_label = "test_label.txt";


	//train :85%  test :10%  validation :5%
	std::ofstream train_f;
	train_f.open(train_label, std::ios::out | std::ios::trunc);
	if (!train_f.is_open()) {
		printf("open file %s error!\n", train_label);
		return 0;
	}

	std::ofstream test_f;
	test_f.open(test_label, std::ios::out | std::ios::trunc);
	if (!test_f.is_open()) {
		printf("open file %s error!\n", test_label);
		return 0;
	}

	std::ofstream val_f;
	val_f.open(val_label, std::ios::out | std::ios::trunc);
	if (!val_f.is_open()) {
		printf("open file %s error!\n", val_label);
		return 0;
	}

	Librosa rosa;
	int num = 0;
	create_data(rosa, total / 2, 4, -120.0f, 0.0f,"adv_wav", "adv_mel", 0, num, train_f, test_f, val_f);
	create_data(rosa, total, 4, 600.0f, 1200.0f, "radio_wav", "radio_mel", 1, num, train_f, test_f, val_f);

	train_f.close();
	test_f.close();
	val_f.close();
	return 1;
}

#if 0
void test_rosa() {
	Librosa rosa;
	std::vector<int16_t> wav;
	std::clock_t begin = clock();
	if (!rosa.load_wav("D12_752.wav", wav)) return;
	cv::Mat_<float> ans = rosa.log_mel(wav, 16000);
	std::clock_t end = clock();
	printf("tims is %d\n", end - begin);
	/*int p = 1;
	for (int i = 0; i < 80; i++)
		printf("(%f,%f,%f) (%f,%f,%f)\n",ans(i, 0), ans(i, 1), ans(i, 2),
			     ans(i, 622), ans(i, 623), ans(i, 624));*/
}

void new_rosa() {
	
	std::vector<int8_t> wav;
	int16_t bit_per_sample, chn;
	int32_t sample_rate;

	Newrosa rosa;
	std::clock_t begin = clock();
	rosa.lame_mp3_to_wav("chun.mp3", "chun2.wav");
	//rosa.merge_wav("13.wav", "chun.wav", "19.wav");
	//rosa.wav_to_mp3("10.wav", "24.mp3");
	//rosa.wav_to_mp3("D12_752.wav", "21.mp3");
	//rosa.mp3_to_wav("chun.mp3","chun.wav");
	//if (!rosa.load_wav("10.wav", wav, bit_per_sample, sample_rate)) return;
	printf("tims is %d \n", clock() - begin);
	return;

	if (!rosa.load_wav("11.wav", wav, bit_per_sample, sample_rate,chn)) return;
	rosa.set_param(sample_rate, bit_per_sample);

	Matrix ans;
	rosa.log_mel(wav, ans);
	printf("tims is %d \n", clock() - begin);
	/*for (int i = 0; i < 80; i++)
		printf("(%f,%f,%f) (%f,%f,%f)\n", ans.data[ans.get_index(i, 0)], ans.data[ans.get_index(i, 1)],
			ans.data[ans.get_index(i, 2)], ans.data[ans.get_index(i, ans.cols - 3)], ans.data[ans.get_index(i, ans.cols - 2)],
			ans.data[ans.get_index(i, ans.cols - 1)]);

	int p = 1;*/
}

void pre_proc() {

	Newrosa rosa;
	
	std::clock_t begin = clock();

	char mp3[128];
	char wav_1[128];
	char wav_2[128];
	char wav_3[128];

	rosa.mp3_to_wav("mp3/1105/1300_2019-11-05_16.mp3", "mp3/632.wav");
	return;

	/*for (int i = 9; i < 21; i++)
	{
		sprintf_s(mp3, "mp3/1300_JinRiHuaTi_%d.mp3", 600 + i);
		sprintf_s(wav_1, "mp3/%d.wav", 600 + i);
		rosa.mp3_to_wav(mp3, wav_1);
	}


		

	strcpy_s(wav_1, "mp3/1105/1.wav");
	strcpy_s(wav_2, "mp3/1105/2.wav");
	strcpy_s(wav_3, "mp3/1105/3.wav");

	rosa.mp3_to_wav("mp3/1105/1300_2019-11-05_3.mp3", wav_1, -190, 0);

	for (int i = 4; i < 26; i++)
	{
		if (i == 11) continue;
		sprintf_s(mp3, "mp3/1105/1300_2019-11-05_%d.mp3", i);
		rosa.mp3_to_wav(mp3, wav_2, -190, 0);
		if (i == 25)
			rosa.merge_wav(wav_1, wav_2, "mp3/1105/1105_jr.wav");
		else
			rosa.merge_wav(wav_1, wav_2, wav_3);
		char temp[128];
		strcpy_s(temp, wav_1);
		strcpy_s(wav_1,wav_3);
		strcpy_s(wav_3, temp);
	}
	strcpy_s(wav_1, "mp3/1022/1.wav");
	strcpy_s(wav_2, "mp3/1022/2.wav");
	strcpy_s(wav_3, "mp3/1022/3.wav");

	rosa.mp3_to_wav("mp3/1022/1300_2019-10-22_3.mp3", wav_1, -190, 0);

	for (int i = 4; i < 26; i++)
	{
		if (i == 11) continue;
		sprintf_s(mp3, "mp3/1022/1300_2019-10-22_%d.mp3", i);
		rosa.mp3_to_wav(mp3, wav_2, -190, 0);
		if (i == 25)
			rosa.merge_wav(wav_1, wav_2, "mp3/1022/1022_jr.wav");
		else
			rosa.merge_wav(wav_1, wav_2, wav_3);
		char temp[128];
		strcpy_s(temp, wav_1);
		strcpy_s(wav_1, wav_3);
		strcpy_s(wav_3, temp);
	}
	*/

	strcpy_s(wav_1, "mp3/1029/1.wav");
	strcpy_s(wav_2, "mp3/1029/2.wav");
	strcpy_s(wav_3, "mp3/1029/3.wav");

	rosa.mp3_to_wav("mp3/1029/1300_2019-10-29_3.mp3", wav_1, -190, 0);

	for (int i = 4; i < 26; i++)
	{
		if (i == 11) continue;
		sprintf_s(mp3, "mp3/1029/1300_2019-10-29_%d.mp3", i);
		rosa.mp3_to_wav(mp3, wav_2, -190, 0);
		if (i == 25)
			rosa.merge_wav(wav_1, wav_2, "mp3/1029/1029_jr.wav");
		else
			rosa.merge_wav(wav_1, wav_2, wav_3);
		char temp[128];
		strcpy_s(temp, wav_1);
		strcpy_s(wav_1, wav_3);
		strcpy_s(wav_3, temp);
	}	

	printf("tims is %d \n", clock() - begin);
}

void array2Mat(std::vector<float> data, int chn, int width, int height, cv::Mat &img) {

	cv::Mat dst_img(width, height, CV_32FC(chn));

#pragma omp parallel for
	for (int h = 0; h < dst_img.rows; h++) {
		float* s = dst_img.ptr<float>(h);
		int img_index = 0;
		for (int w = 0; w < dst_img.cols; w++) {
			for (int c = 0; c < dst_img.channels(); c++) {
				int data_index = c * dst_img.rows * dst_img.cols + h * dst_img.cols + w;
				s[img_index++] = data[data_index];
			}
		}
	}
	cv::normalize(dst_img, dst_img, 0, 1, CV_MINMAX);
	cv::convertScaleAbs(dst_img, img, 255);
}

int create_data(int step=10) {
	/*const char* srcfile[] = { "mp3/609.wav", "mp3/610.wav","mp3/611.wav","mp3/612.wav","mp3/613.wav","mp3/614.wav",
							  "mp3/615.wav", "mp3/616.wav","mp3/617.wav","mp3/618.wav","mp3/619.wav","mp3/620.wav",
		                      "mp3/1105_jr.wav", "mp3/1029_jr.wav", "mp3/1022_jr.wav"};*/


	const char* srcfile[] = { "mp3/632.wav" };
	const char* mel_path = "wav_data";
	const char* train_label = "train_label.txt";
	const char* val_label = "val_label.txt";
	const char* test_label = "test_label.txt";


	//train :85%  test :10%  validation :5%
	srand(time(NULL));
	std::ofstream train_f;
	train_f.open(train_label, std::ios::out | std::ios::trunc);
	if (!train_f.is_open()) {
		printf("open file %s error!\n", train_label);
		return 0;
	}

	std::ofstream test_f;
	test_f.open(test_label, std::ios::out | std::ios::trunc);
	if (!test_f.is_open()) {
		printf("open file %s error!\n", test_label);
		return 0;
	}

	std::ofstream val_f;
	val_f.open(val_label, std::ios::out | std::ios::trunc);
	if (!val_f.is_open()) {
		printf("open file %s error!\n", val_label);
		return 0;
	}

	Newrosa rosa;
	std::vector<int8_t> wav;
	int16_t bit_per_sample, chn;
	int32_t sample_rate;
	char mel_file[128];
	char mel_all_path[128];
	char img_path[128];
	const char *temp = "temp";
	std::ofstream out;
	int id = 0;
	//printf("%d  %d %s   %s \n", sizeof(srcfile),sizeof(void*), srcfile[0], srcfile[1]);

	for (int i = 0; i < sizeof(srcfile)/ sizeof(void*); i++) {		
		int start = 0;		
		while (1) {
			
			sprintf_s(mel_file, "wav_%04d.wav", id);
			sprintf_s(mel_all_path, "%s/%s", mel_path, mel_file);
			if (!rosa.cut_wav(srcfile[i], temp, start, start + step)) break;			
			rosa.stero2mono(temp, mel_all_path);
			
			if (!rosa.load_wav(mel_all_path, wav, bit_per_sample, sample_rate, chn)) break;
			if (id == 0) rosa.set_param(sample_rate, bit_per_sample);
			Matrix ans;
			rosa.log_mel(wav, ans);
			ans.transpos();
			//printf("%d  %d \n", ans.rows, ans.cols);
			cv::Mat img(ans.rows/2, 160, CV_32FC(1), ans.data.data());
			//cv::Mat img1(ans.rows, ans.cols, CV_32FC(1));
			//array2Mat(ans.data, 1, ans.rows, ans.cols, img1);
			//sprintf_s(img_path, "img_data/1wav_% 04d.jpg", id);
			//cv::imwrite(img_path, img1);
			//cv::imshow("ok", img1);
			//cv::waitKey(0);

			cv::Mat resultImage;
			cv::normalize(img, resultImage, 0, 1, CV_MINMAX);
			cv::convertScaleAbs(resultImage, resultImage, 255);
			//cv::imshow("ok", img);
			//cv::waitKey(0);

			sprintf_s(img_path, "img_data/wav_% 04d.jpg", id);
			cv::imwrite(img_path, resultImage);
			//create mel data
			/*if (!rosa.load_wav(srcfile[i], wav, bit_per_sample, sample_rate, chn, start, start + step)) break;
			out.open(mel_all_path, std::ios::out | std::ios::trunc | std::ios::binary);
			if (!out.is_open()) {
				std::cout << mel_all_path << " open err " << std::endl;
				return 0;
			}
			if(id == 0) rosa.set_param(sample_rate, bit_per_sample);
			Matrix ans;
			rosa.log_mel(wav, ans);
			printf("%d  %d \n", ans.rows, ans.cols);
			out.write((char*)ans.data.data(), ans.data.size()*sizeof(float));
			out.write((char*)wav.data(), wav.size());
			out.close();
			*/

			int rnd = rand() % 100;
			int label = i < 1 ? 1 : 0;
			if (rnd < 85) 
				train_f << mel_file << " " << label << std::endl;
			else if(rnd < 95)
				test_f << mel_file << " " << label << std::endl;
			else
				val_f << mel_file << " " << label << std::endl;

			id++;
			start += step;
			//if (id >= 10) break;
		}
	}
	train_f.close();
	test_f.close();
	val_f.close();
}
#endif

int  main()
{
	srand(time(NULL));
	//test_audio();
	//create_wav();
	//create_adv(100, 7);
	create_label(2000);
	return 1;
}



