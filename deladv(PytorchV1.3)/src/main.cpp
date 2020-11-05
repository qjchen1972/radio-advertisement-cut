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
struct del_adv_t {
	std::string path;
	int radio_num;
	int8_t pro_list[32];
};
#pragma pack(pop)


static del_adv_t del_list[] = { {"radio/1300/mp3",1300,{8,9,10,11,12,13,14,15,16,17,18,19,20,22,23,-1}} };
                              

static std::string  home_path = "/home/ubuntu/";

static std::string  init_net = "adv/models/init.pb";
static std::string  pred_net = "adv/models/pred.pb";
static std::string  temp_wav1 = "adv/bin/temp1.wav";
static std::string  temp_wav2 = "adv/bin/temp2.wav";


std::string  get_yesterday() {

	time_t now;
	time(&now);
	now -= 24 * 3600;
	struct tm *pt = localtime(&now);

	char str[128];
	sprintf(str, "%04d-%02d-%02d", 1900 + pt->tm_year, 1 + pt->tm_mon, pt->tm_mday);
	std::string temp = str;
	return temp;
}

/*static int  ai_del_one_adv(Librosa &rosa, const char *srcmp3, int step, const char *dstmp3) {

	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav;

	if (!audio.mp3_to_wav(srcmp3, temp_wav1.c_str())) return 0;
	if (!audio.wav_stero2mono(temp_wav1.c_str(), temp_wav2.c_str())) return 0;
	if (!audio.wav2vec(temp_wav2.c_str(), wav, head)) return 0;

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
		if (!rosa.log_mel(data, ans)) continue;
		ans.transpos();
		if (notAdv(ans.data)) 
			radio_wav.insert(radio_wav.end(), data.begin(), data.end());
		else 
			adv_wav.insert(adv_wav.end(), data.begin(), data.end());
		start += buflen;
	}

	if (!radio_wav.empty()) {
		if (!audio.vec2wav(radio_wav.data(), radio_wav.size(), temp_wav1.c_str(), head.bit_per_sample, head.sr, head.chn))
			return 0;
		if (!audio.wav_mono2stero(temp_wav1.c_str(), temp_wav2.c_str()))
			return 0;
		return audio.wav_to_mp3(temp_wav2.c_str(), dstmp3);
	}
	return 0;
}
*/

static int  ai_del_one_adv(Librosa &rosa, const char *srcmp3, int step, const char *dstmp3) {

	Audio audio;
	wav_head_t head;
	std::vector<int8_t> wav;
	AdvVerify adv(11);

	if (!audio.mp3_to_wav(srcmp3, temp_wav1.c_str())) return 0;
	if (!audio.wav_stero2mono(temp_wav1.c_str(), temp_wav2.c_str())) return 0;
	if (!audio.wav2vec(temp_wav2.c_str(), wav, head)) return 0;

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
		if (!rosa.log_mel(data, ans)) continue;
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
		if (!audio.vec2wav(radio_wav.data(), radio_wav.size(), temp_wav1.c_str(), head.bit_per_sample, head.sr, head.chn))
			return 0;
		if (!audio.wav_mono2stero(temp_wav1.c_str(), temp_wav2.c_str()))
			return 0;
		return audio.wav_to_mp3(temp_wav2.c_str(), dstmp3);
	}
	return 0;
}


int del_adv() {

	char src_file[128];
	char deladv_file[128];

	init_net = home_path + init_net;
	pred_net = home_path + pred_net;
	temp_wav1 = home_path + temp_wav1;
	temp_wav2 = home_path + temp_wav2;


	if (!initMel(init_net.c_str(), pred_net.c_str())) return 0;

	std::string  yesterday = get_yesterday();	
	Librosa rosa;

	for (int i = 0; i < sizeof(del_list) / sizeof(del_adv_t); i++) {
		del_list[i].path = home_path + del_list[i].path;
		for (int j = 0; j < sizeof(del_list[i].pro_list) / sizeof(int8_t); j++) {
			if (del_list[i].pro_list[j] < 0) break;			
			sprintf(src_file, "%s/%s/%d_%s_%d.mp3", 
				              del_list[i].path.c_str(),
				              yesterday.c_str(),
				              del_list[i].radio_num, 
				              yesterday.c_str(),
				              del_list[i].pro_list[j]);

			sprintf(deladv_file, "%s/%s/%d_%s_%d_deladv.mp3",
				del_list[i].path.c_str(),
				yesterday.c_str(),
				del_list[i].radio_num,
				yesterday.c_str(),
				del_list[i].pro_list[j]);
			//printf("src %s  dst  %s \n", src_file, deladv_file);
			if (!ai_del_one_adv(rosa, src_file, 4, deladv_file)) {
				printf("%s to  %s error!\n", src_file, deladv_file);
				continue;
			}
		}		
	}
}


int main(int argc, char **argv) {	
	return del_adv();	
}
