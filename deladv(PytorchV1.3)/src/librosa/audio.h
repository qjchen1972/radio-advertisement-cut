#pragma once
#include<math.h>
#include <iostream>
#include <fstream>
#include <vector>

#ifdef __unix
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
#define SHRT_MAX __SHRT_MAX__
#define FLT_EPSILON __FLT_EPSILON__
#define sprintf_s  sprintf
#endif

//libmp3lame
#include "lame.h"

//ref:https://github.com/lieff/minimp3
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#define MINIMP3_SKIP_ID3V1
#include "minimp3_ex.h"



#pragma  pack (push,1)
struct wav_head_t {
	char mark[4]; //以'RIFF'为标识
	int32_t file_size; //整个文件的长度减去ID和Size的长度
	char type[4]; //WAVE 表示后面需要两个子块：Format区块和Data区块
	char fmt[4];//以'fmt '为标识
	int32_t fmt_len; //fmt区块数据的长度（不包含fmt和Size的长度）
	int16_t fmt_type; //表示Data区块存储的音频数据的格式，PCM音频数据的值为1
	int16_t chn; //表示音频数据的声道数，1：单声道，2：双声道
	int32_t sr; //sample rate 音频数据的采样率
	int32_t byte_rate; //每秒数据字节数 sr * chn * bit_per_sample / 8
	int16_t block_align; //每个采样所需的字节数 = chn * bit_per_sample / 8
	int16_t bit_per_sample; //每个采样存储的bit数，8：8bit，16：16bit，32：32bit
	char chunk[4]; //以'data'为标识
	int32_t size; //表示音频数据的长度，N = ByteRate * seconds
};
#pragma pack(pop)

class Audio {
public:
	Audio() {}
	~Audio() {}

	int vec2wav(int8_t* buf, int len, const char* fname, int16_t bit_per_sample, int32_t sample_rate, int16_t chn) {

		std::ofstream out;
		out.open(fname, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!out.is_open()) {
			std::cout << fname << " open err " << std::endl;
			return 0;
		}
		wav_head_t head = create_head(sample_rate, chn, bit_per_sample, len);
		out.write((char*)&head, sizeof(head));
		out.write((char*)buf, len);
		out.close();
		return 1;
	}

	int wav2vec(const char* fname, std::vector<int8_t> &ret, wav_head_t &head, float start = 0.0f, float end = 0.0f) {

		std::ifstream of;
		of.open(fname, std::ios::in | std::ios::binary);
		if (!of.is_open()) {
			std::cout << fname << " open err " << std::endl;
			return 0;
		}

		of.read((char*)&head, sizeof(head));
		int32_t byte_rate = head.bit_per_sample * head.chn * head.sr >> 3;

		int32_t start_point = get_pos(start, head.size, byte_rate, 0);
		int32_t end_point = get_pos(end, head.size, byte_rate, head.size);		
		int wav_len = end_point - start_point;
		if (wav_len <= 0) return 0;

		ret.resize(wav_len);
		of.seekg(sizeof(head) + start_point);
		of.read((char*)ret.data(), wav_len);
		of.close();
		return 1;
	}

	int wav_to_mp3(const char *input_file_name, const char *output_file_name, float start = 0.0f, float end = 0.0f) {

		std::vector<int8_t> wav;
		wav_head_t head;
		if (!wav2vec(input_file_name, wav, head, start, end)) return 0;

		FILE *out;
		if (fopen_s(&out, output_file_name, "wb+") != 0) {
			std::cout << output_file_name << " open err " << std::endl;
			return 0;
		}

		lame_t lame = lame_init();
		lame_set_in_samplerate(lame, head.sr);
		lame_set_num_channels(lame, head.chn);
		//lame_set_brate(lame, 128);

		//使用动态码率文件比较小
		//lame_set_VBR(lame, vbr_default);
		//输入流没办法回溯，需要设置为0
		lame_set_bWriteVbrTag(lame, 0);

		lame_init_params(lame);

		size_t num_samples = wav.size() >> 1;
		if (head.chn == 2) num_samples >>= 1;


		uint8_t *mp3 = new uint8_t[wav.size()];
		int num;
		if (head.chn == 1) //单声道
			num = lame_encode_buffer(lame, (int16_t*)wav.data(), NULL, num_samples, mp3, wav.size());
		else  //多声道
			num = lame_encode_buffer_interleaved(lame, (int16_t*)wav.data(), num_samples, mp3, wav.size());

		if (num < 0) {
			delete[] mp3;
			fclose(out);
			return 0;
		}

		if (num > 0)
			fwrite((char*)mp3, num, 1, out);
		num = lame_encode_flush(lame, mp3, wav.size());
		if (num > 0)
			fwrite((char*)mp3, num, 1, out);

		lame_mp3_tags_fid(lame, out);
		lame_close(lame);
		delete[] mp3;
		fclose(out);
		return 1;
	}

	int mp3_to_wav(const char *input_file_name, const char *output_file_name, float start = 0.0f, float end = 0.0f) {

		mp3dec_t mp3d;
		mp3dec_file_info_t info;
		if (mp3dec_load(&mp3d, input_file_name, &info, 0, 0)) {
			return 0;
		}

		std::ofstream out;
		out.open(output_file_name, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!out.is_open()) {
			std::cout << output_file_name << " open err " << std::endl;
			return 0;
		}
		int32_t byte_rate = 16 * info.channels * info.hz >> 3;
		int32_t len = info.samples * sizeof(mp3d_sample_t);

		int32_t start_point = get_pos(start, len, byte_rate, 0);
		int32_t end_point = get_pos(end, len, byte_rate, len);
		int wav_len = end_point - start_point;
		if (wav_len <= 0) return 0;


		wav_head_t head = create_head(info.hz, info.channels, 16, wav_len);
		out.write((char*)&head, sizeof(head));
		out.write((char*)info.buffer + start_point, wav_len);
		free(info.buffer);
		out.close();
	}

	int  wav_stero2mono(const char* fname1, const char* fname2) {

		std::ifstream of;
		of.open(fname1, std::ios::in | std::ios::binary);
		if (!of.is_open()) {
			std::cout << fname1 << " open err " << std::endl;
			return 0;
		}

		wav_head_t head;
		of.read((char*)&head, sizeof(head));
		std::vector<int16_t> ret(head.size / 2);
		of.read((char*)ret.data(), head.size);
		of.close();

		std::ofstream out;
		out.open(fname2, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!out.is_open()) {
			std::cout << fname2 << " open err " << std::endl;
			return 0;
		}

		std::vector<int16_t> wret(ret.size() / 2);
#pragma omp parallel for
		for (int i = 0; i < wret.size(); i++) {

			wret[i] = (static_cast<int32_t>(ret[2 * i]) + ret[2 * i + 1]) >> 1;
		}

		wav_head_t whead = create_head(head.sr, head.chn / 2, head.bit_per_sample, wret.size() * 2);
		out.write((char*)&whead, sizeof(whead));
		out.write((char*)wret.data(), wret.size() * 2);
		out.close();
		return 1;
	}

	int  wav_mono2stero(const char* fname1, const char* fname2) {

		std::ifstream of;
		of.open(fname1, std::ios::in | std::ios::binary);
		if (!of.is_open()) {
			std::cout << fname1 << " open err " << std::endl;
			return 0;
		}

		wav_head_t head;
		of.read((char*)&head, sizeof(head));
		std::vector<int16_t> ret(head.size / 2);
		of.read((char*)ret.data(), head.size);
		of.close();
		if (head.chn > 1) return 0;

		std::ofstream out;
		out.open(fname2, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!out.is_open()) {
			std::cout << fname2 << " open err " << std::endl;
			return 0;
		}

		std::vector<int16_t> wret(ret.size() * 2);
#pragma omp parallel for
		for (int i = 0; i < ret.size(); i++) {

			wret[2 * i] = ret[i];
			wret[2 * i + 1] = ret[i];
		}

		wav_head_t whead = create_head(head.sr, head.chn * 2, head.bit_per_sample, wret.size() * 2);
		out.write((char*)&whead, sizeof(whead));
		out.write((char*)wret.data(), wret.size() * 2);
		out.close();
		return 1;
	}

	//切割
	int cut_wav(const char* infile, const char* outfile, float start = 0.0f, float end = 0.0f) {

		std::vector<int8_t> wav;
		wav_head_t head;
		if (!wav2vec(infile, wav, head, start, end)) return 0;
		if (!vec2wav(wav.data(), wav.size(), outfile, head.bit_per_sample, head.sr, head.chn)) return 0;
		return 1;
	}

	//合并
	int merge_wav(const char* firstfile, const char* secondfile, const char* outfile) {


		std::vector<int8_t> first_wav, second_wav;
		wav_head_t first_head,second_head;

		if (!wav2vec(firstfile, first_wav, first_head)) return 0;
		if (!wav2vec(secondfile, second_wav, second_head)) return 0;
		first_wav.insert(first_wav.end(), second_wav.begin(), second_wav.end());
		if (!vec2wav(first_wav.data(), first_wav.size(), outfile, first_head.bit_per_sample, first_head.sr, first_head.chn)) 
			return 0;
		return 1;	
	}

	//合成
	int compose_wav(const char* firstfile, const char* secondfile, const char* outfile) {


		std::vector<int8_t> first_wav, second_wav;
		wav_head_t first_head, second_head;

		if (!wav2vec(firstfile, first_wav, first_head)) return 0;
		if (!wav2vec(secondfile, second_wav, second_head)) return 0;

		int len = first_wav.size() > second_wav.size() ? second_wav.size() : first_wav.size();
		std::vector<int16_t> ret(len >> 1);
#pragma omp parallel for
		for (int i = 0; i < ret.size(); i++) {
			int temp = (int)*((int16_t*)(first_wav.data() + 2 * i)) + (int)*((int16_t*)(second_wav.data() + 2 * i));
			if (temp > SHRT_MAX) 
				ret[i] = SHRT_MAX;
			else
				ret[i] = temp;
		}
		if (!vec2wav((int8_t*)ret.data(), ret.size() * 2, outfile, first_head.bit_per_sample, first_head.sr, first_head.chn))
			return 0;
		return 1;
	}

private:

	wav_head_t create_head(int hz, int ch, int bips, int data_bytes)
	{
		wav_head_t head;
		memcpy(head.mark, "RIFF", sizeof(head.mark));
		head.file_size = sizeof(head) + data_bytes - 8;
		memcpy(head.type, "WAVE", sizeof(head.type));
		memcpy(head.fmt, "fmt ", sizeof(head.fmt));
		head.fmt_len = 16;
		head.fmt_type = 1;
		head.chn = ch;
		head.sr = hz;
		head.byte_rate = bips * ch * hz >> 3;
		head.block_align = bips * ch >> 3;
		head.bit_per_sample = bips;
		memcpy(head.chunk, "data", sizeof(head.chunk));
		head.size = data_bytes;
		return head;
	}

	int32_t get_pos(float second, int32_t total_len, int32_t byte_rate, int32_t init_value) {
		if (second > FLT_EPSILON) {
			int temp = (int)second * byte_rate;
			return temp > total_len ? total_len : temp;
		}
		else if (second < -FLT_EPSILON) {
			int temp = total_len + (int)second * byte_rate;
			return temp > total_len ? total_len : temp;
		}
		else
			return init_value;
	}

	//使用lame解码，不能工作。。。，没时间研究
	/*int lame_mp3_to_wav(const char *input_file_name, const char *output_file_name) {

		std::ifstream of;
		of.open(input_file_name, std::ios::in | std::ios::binary);
		if (!of.is_open()) {
			std::cout << input_file_name << " open err " << std::endl;
			return 0;
		}

		std::ofstream out;
		out.open(output_file_name, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!out.is_open()) {
			std::cout << output_file_name << " open err " << std::endl;
			return 0;
		}

		const int PCM_SIZE = 8192;
		const int MP3_SIZE = 8192;

		// 输出左右声道
		short int pcm_l[PCM_SIZE], pcm_r[PCM_SIZE];
		unsigned char mp3_buffer[MP3_SIZE];

		lame_t lame = lame_init();
		lame_set_decode_only(lame, 1);

		hip_t hip = hip_decode_init();

		mp3data_struct mp3data;
		memset(&mp3data, 0, sizeof(mp3data));
		wav_head_t head;
		memset(&head, 0, sizeof(head));

		int data_size = 0;
		while (1) {
			int mp3_len = of.read((char*)mp3_buffer, sizeof(mp3_buffer)).gcount();
			if (of.fail()) break;
			int samples = hip_decode_headers(hip, mp3_buffer, mp3_len, pcm_l, pcm_r, &mp3data);
			if (mp3data.header_parsed == 1)//header is gotten
			{
				head.sr = mp3data.samplerate;
				head.chn = mp3data.stereo;
				head.bit_per_sample = 16;// mp3data.bitrate * 8 / (mp3data.samplerate * mp3data.stereo);

			}
			if (samples > 0) {
				for (int i = 0; i < samples; i++) {
					out.write((char*)&pcm_l[i], sizeof(pcm_l[i]));
					if (head.chn == 2)
						out.write((char*)&pcm_r[i], sizeof(pcm_r[i]));
				}
				if (head.chn == 2)
					data_size += 2 * samples;
				else
					data_size += samples;
			}
		}
		hip_decode_exit(hip);
		lame_close(lame);
		out.seekp(0, std::ios::beg);
		head = create_head(head.sr, head.chn, head.bit_per_sample, data_size >> 1);
		out.write((char*)&head, sizeof(head));
		out.close();
		of.close();
	}*/
};