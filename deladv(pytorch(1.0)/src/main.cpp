#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include<ctime>
//#include "opencv2/opencv.hpp"
#include "librosa/audio.h"
#include "librosa/librosa.h"
#include "mel/mel.h"
//#include "utils/torch_net.h"
#include "adv_verify.h"

#pragma  pack (push,1)
struct audio_path_t {
	const char* path;
	int month;
	int day;
};
#pragma pack(pop)


static audio_path_t audio_path[] = { {"D:/hq_work/wav_transformer/mel/mp3/1022", 10,22},
							  {"D:/hq_work/wav_transformer/mel/mp3/1029",10,29},
							  {"D:/hq_work/wav_transformer/mel/mp3/1105",11,5},
							  {"D:/hq_work/wav_transformer/mel/mp3/1112",11,12} };
static int audio_path_len = sizeof(audio_path) / sizeof(audio_path_t);

static const char *init_net = "../models/init.pb";
static const char *pred_net = "../models/pred.pb";



static void ai_create_wav(Librosa &rosa, const char *wav_file, const char *adv_file, const char *radio_file, int step) {
	
	Audio audio;	
	wav_head_t head;
	std::vector<int8_t> wav;
	if (!audio.wav2vec(wav_file, wav, head)) return;

	int32_t byte_rate = head.bit_per_sample * head.chn * head.sr >> 3;
	int32_t buflen = step * byte_rate;	
	std::vector<int8_t> data(buflen);
	std::vector<int8_t> adv_wav;
	std::vector<int8_t> radio_wav;

	rosa.set_param(head.sr, head.bit_per_sample);

	int start = 0;
	while (1) {
		if (start > wav.size() - buflen) break;
		memcpy(data.data(), wav.data() + start, buflen);
		Matrix ans;
		rosa.log_mel(data, ans);
		ans.transpos();
		if (notAdv(ans.data)) 
			radio_wav.insert(radio_wav.end(), data.begin(), data.end());		
		else 
			adv_wav.insert(adv_wav.end(), data.begin(), data.end());
		start += buflen;
	}

	audio.vec2wav(radio_wav.data(), radio_wav.size(), radio_file, head.bit_per_sample, head.sr, head.chn);
	audio.vec2wav(adv_wav.data(), adv_wav.size(), adv_file, head.bit_per_sample, head.sr, head.chn);
	return;
}

static void create_wav(int step) {
	char mp3[128];
	char adv_wav[128];
	char radio_wav[128];

	Audio audio;
	Librosa rosa;

	for (int i = 0; i < audio_path_len; i++) {
		for (int j = 9; j < 25; j++) {
			sprintf_s(mp3, "%s/1300_2019-%02d-%02d_%d.mp3",
				audio_path[i].path,
				audio_path[i].month,
				audio_path[i].day,
				j);

			sprintf_s(adv_wav, "%s/adv_%d.wav", audio_path[i].path, j - 9);
			sprintf_s(radio_wav, "%s/radio_%d.wav", audio_path[i].path, j - 9);
			audio.mp3_to_wav(mp3, "temp1.wav");
			audio.wav_stero2mono("temp1.wav", "temp.wav");
			ai_create_wav(rosa, "temp.wav", adv_wav, radio_wav, step);
		}
	}
}

/*static void ai_del_adv(const char *mp3, int step) {
	
	Librosa rosa;
	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav;

	audio.mp3_to_wav(mp3, "wav1.wav");
	audio.wav_stero2mono("wav1.wav", "wav.wav");

	if (!audio.wav2vec("wav.wav", wav, head)) return;

	//int32_t byte_rate = head.bit_per_sample * head.chn * head.sr >> 3;
	int32_t buflen = step * head.byte_rate;
	std::vector<int8_t> data(buflen);
	std::vector<int8_t> adv_wav;
	std::vector<int8_t> radio_wav;

	rosa.set_param(head.sr, head.bit_per_sample);

	int start = 0;
	while (1) {
		if (start > wav.size() - buflen) break;
		memcpy(data.data(), wav.data() + start, buflen);
		Matrix ans;
		rosa.log_mel(data, ans);
		ans.transpos();
		if (notAdv(ans.data)) {
			radio_wav.insert(radio_wav.end(), data.begin(), data.end());
			//printf("radio len = %d \n", radio_wav.size());
		}
		else {
			adv_wav.insert(adv_wav.end(), data.begin(), data.end());
			//printf("adv len = %d \n", adv_wav.size());
		}
		start += buflen;
	}

	if (!radio_wav.empty()) {
		audio.vec2wav(radio_wav.data(), radio_wav.size(), "radio.wav", head.bit_per_sample, head.sr, head.chn);
		audio.wav_to_mp3("radio.wav", "radio.mp3");
	}
	if (!adv_wav.empty()) {
		audio.vec2wav(adv_wav.data(), adv_wav.size(), "adv.wav", head.bit_per_sample, head.sr, head.chn);
		audio.wav_to_mp3("adv.wav", "adv.mp3");
	}

	return;
}
*/

static void ai_del_adv(const char *mp3, int step) {

	Librosa rosa;
	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav;
	AdvVerify adv(11);

	audio.mp3_to_wav(mp3, "wav1.wav");
	audio.wav_stero2mono("wav1.wav", "wav.wav");

	if (!audio.wav2vec("wav.wav", wav, head)) return;

	//int32_t byte_rate = head.bit_per_sample * head.chn * head.sr >> 3;
	int32_t buflen = step * head.byte_rate;
	std::vector<int8_t> data(buflen);
	std::vector<int8_t> adv_wav;
	std::vector<int8_t> radio_wav;

	rosa.set_param(head.sr, head.bit_per_sample);

	int start = 0;
	while (1) {
		if (start > wav.size() - buflen) break;
		memcpy(data.data(), wav.data() + start, buflen);
		Matrix ans;
		rosa.log_mel(data, ans);
		ans.transpos();

		adv_verify_t ver;
		ver.wav = data;
		ver.type = notAdv(ans.data) ? 1 : -1;
		adv.push(ver);
		if (adv.pop(ver)) {
			if (ver.type > 0)
				radio_wav.insert(radio_wav.end(), ver.wav.begin(), ver.wav.end());
			else
				adv_wav.insert(adv_wav.end(), ver.wav.begin(), ver.wav.end());
		}
		start += buflen;
	}
	std::vector<adv_verify_t> wavs;
	adv.get_all(wavs);
	for (int i = 0; i < wavs.size(); i++) {
		if (wavs[i].type > 0)
			radio_wav.insert(radio_wav.end(), wavs[i].wav.begin(), wavs[i].wav.end());
		else
			adv_wav.insert(adv_wav.end(), wavs[i].wav.begin(), wavs[i].wav.end());
	}

	if (!radio_wav.empty()) {
		audio.vec2wav(radio_wav.data(), radio_wav.size(), "radio.wav", head.bit_per_sample, head.sr, head.chn);
		audio.wav_to_mp3("radio.wav", "radio.mp3");
	}
	if (!adv_wav.empty()) {
		audio.vec2wav(adv_wav.data(), adv_wav.size(), "adv.wav", head.bit_per_sample, head.sr, head.chn);
		audio.wav_to_mp3("adv.wav", "adv.mp3");
	}

	return;
}

void delAdv() {

	const char *init_net = "../models/init.pb";
	const char *pred_net = "../models/pred.pb";
	const char *mp3 = "1300_2019-11-14_10.mp3";
	
	//先初始化
	if (!initMel(init_net, pred_net)) return;

	Librosa rosa;
	Audio audio;

	std::clock_t begin = clock();
	audio.mp3_to_wav(mp3, "wav1.wav");
	audio.wav_stero2mono("wav1.wav", "wav.wav");	

	wav_head_t head;
	std::vector<int8_t> wav;
	if (!audio.wav2vec("wav.wav", wav, head)) return ;

	int32_t byte_rate = head.bit_per_sample * head.chn * head.sr >> 3;
	int32_t fivelen = 4 * byte_rate;
	int32_t mellen = 160 * 160;
	//std::vector<float> mel(mellen,0.0f);
	int start = 0;
	int reallen = 0;
	std::vector<int8_t> data(fivelen);
	int8_t *real = new int8_t[wav.size()];
	printf("wav size = %d , %d  %d  %d\n", wav.size(), head.bit_per_sample, head.sr, head.chn);
	rosa.set_param(head.sr, head.bit_per_sample);
	int del = 0;
	while (1) {
		if (start > wav.size() - fivelen) break;
		memcpy(data.data(), wav.data() + start, fivelen);
		Matrix ans;
		rosa.log_mel(data, ans);

		/*for (int i = 0; i < ans.rows; i++) {
			for (int j = 0; j < ans.cols; j++)
				printf("%f,", ans.data[i*ans.cols + j]);
			printf("\n");
		}
		return;*/

		ans.transpos();
		if (!notAdv(ans.data)) {
			memcpy(real + reallen, data.data(), fivelen);
			reallen += fivelen;
			//printf("%d is adv \n", start / fivelen);
		}
		else del++;
		//else
			//printf("%d is not adv \n", start / fivelen);
		start += fivelen;
	}
	audio.vec2wav(real, reallen, "deladv.wav", head.bit_per_sample, head.sr, head.chn);
	//audio.wav_mono2stero("deladv1.wav", "deladv.wav");
	audio.wav_to_mp3("deladv.wav", "adv.mp3");
	printf("del %d \n", del);
	return;
}

void cal_one(const char *wav_file) {
	Librosa rosa;
	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav;

	if (!audio.wav2vec(wav_file, wav, head)) return;
	Matrix ans;
	rosa.set_param(head.sr, head.bit_per_sample);
	rosa.log_mel(wav, ans);
	ans.transpos();
	printf("%d  %d \n", ans.rows, ans.cols);
	/*for (int i = 0; i < ans.rows; i++) {
		for (int j = 0; j < ans.cols; j++) {
			ans.data[i*ans.cols + j] = 1.0;
			if (i == 0 && j == 0)  ans.data[i*ans.cols + j] = 10.0;
			if (i == 1 && j == 1)  ans.data[i*ans.cols + j] = 10.0;
		}

				//printf("%f,", ans.data[i*ans.cols + j]);
			//printf("\n");
		}*/

	//std::vector<float> data;
	//data.insert(data.end(), ans.data.begin(), ans.data.begin() + 160*160*sizeof(float));

	std::clock_t begin = clock();
	notAdv(ans.data);
	printf("caffe time = %d \n", clock() - begin);
}

#if 0
void torch_cal_one(Torch_net &net, const char *wav_file) {
	Librosa rosa;
	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav;

	if (!audio.wav2vec(wav_file, wav, head)) return;
	Matrix ans;
	rosa.set_param(head.sr, head.bit_per_sample);
	rosa.log_mel(wav, ans);
	ans.transpos();
	printf("%d  %d \n", ans.rows, ans.cols);
	/*for (int i = 0; i < ans.rows; i++) {
			for (int j = 0; j < ans.cols; j++)
				printf("%f,", ans.data[i*ans.cols + j]);
			printf("\n");
		}*/

		//std::vector<float> data;
		//data.insert(data.end(), ans.data.begin(), ans.data.begin() + 160*160*sizeof(float));
	std::vector<float> out;
	net.forward(ans.data, out);

}
#endif

void test_fft() {
	Librosa rosa;
	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav;

	if (!audio.wav2vec("D12_752.wav", wav, head)) return ;
	printf("%d , %d \n", wav.size(), head.sr);
	rosa.set_param(head.sr, head.bit_per_sample);
	Matrix ans;
	rosa.log_mel(wav, ans);
	ans.show("log_mel:");
}


int main(int argc, char **argv) {

	//test_fft();
	//return 1;
	
	//先初始化
	if (!initMel(init_net, pred_net)) return 0;

	create_wav(4);
	ai_del_adv("1300_2019-11-12_19.mp3", 4);
	delAdv();
	cal_one("4.wav");

	//Torch_net net(160, 160);
	//net.initNet("../models/model.pt");
	//torch_cal_one(net, "4.wav");
	//Audio audio;
	//audio.wav_to_mp3("4.wav", "4.mp3");
	//audio.mp3_to_wav("4.mp3", "5.wav");

	return 0;
}
