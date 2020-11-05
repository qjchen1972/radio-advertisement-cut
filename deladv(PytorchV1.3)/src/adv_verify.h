#pragma once

#include <vector>
#include<list>

#pragma  pack (push,1)
struct adv_verify_t {
	int8_t type;
	std::vector<int8_t> wav;
	int16_t before_type;
	int16_t after_type;
};
#pragma pack(pop)

class AdvVerify {
public:
	AdvVerify(int len):size(len){}
	~AdvVerify() {}

	void push(adv_verify_t wav) {
	
		if (wav_list.size() < 1) {
			wav.before_type = 0;
			wav.after_type = 0;
			wav_list.push_back(wav);
			return;
		}
		std::list<adv_verify_t>::iterator iter;
		wav.before_type = 0;
		for (iter = wav_list.begin(); iter != wav_list.end(); iter++) {
			wav.before_type += iter->type;
			iter->after_type += wav.type;
		}
		wav.after_type = 0;
		wav_list.push_back(wav);
	}

	int pop(adv_verify_t &wav) {
		if (wav_list.size() < size) return 0;
		wav = wav_list.front();
		wav_list.pop_front();	
		proc_type(wav);
		return 1;
	}

	void get_all(std::vector<adv_verify_t> &wavs) {
		std::list<adv_verify_t>::iterator iter;
		for (iter = wav_list.begin(); iter != wav_list.end(); iter++) {

			adv_verify_t one = *iter;
			proc_type(one);
			wavs.push_back(one);
		}
	}

private:
	std::list<adv_verify_t> wav_list;
	int size = 11;

	int set_type( int  type) {
		if (type > 0) type = 1;
		else if (type < 0) type = -1;
		else type = 0;
		return type;
	}

	void proc_type(adv_verify_t &wav) {
		wav.after_type = set_type(wav.after_type);
		wav.before_type = set_type(wav.before_type);
		if (wav.before_type != 0 && (wav.before_type == wav.after_type)) {
			if (wav.type != wav.before_type) {
				wav.type = wav.before_type;
				//printf("modify it  %d\n ", wav.type);
			}
			return ;
		}
		if (wav.after_type == 0 && wav.before_type != 0)
			wav.type = wav.before_type;
		if (wav.before_type == 0 && wav.after_type != 0)
			wav.type = wav.after_type;
	}
};