#pragma once
#include<math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cblas.h>
#include "AudioFFT.h"
#include "matrix.h"

class Librosa {

public:
	Librosa(){}
	~Librosa(){}	

	void set_param(int32_t sample_rate = 16000, int16_t bit_per_sample = 16) {
		nSamplesPerSec = sample_rate;
		byte_per_sample = bit_per_sample / (sizeof(int8_t)*8);
		hop_length = int(0.01 * nSamplesPerSec); //0.0125
		win_length = int(0.04 * nSamplesPerSec); //0.05
		length_DFT = 2048;
	}

	void set_data(std::vector<int8_t> &data) {
	
		switch (byte_per_sample) {
		case 1:
			fdata.resize(data.size());
#pragma omp parallel for
			for (int i = 0; i < fdata.size(); i++)
				fdata[i] = data[i];
			return;
		case 2:
		{
			std::vector<int16_t> temp;
			temp.resize(data.size() / 2);
			memcpy(temp.data(), data.data(), data.size());
			fdata.resize(temp.size());
#pragma omp parallel for
			for (int i = 0; i < fdata.size(); i++) 
				fdata[i] = temp[i];
			
			return;
		}
		case 4:
		{
			{
				std::vector<int32_t> temp;
				temp.resize(data.size() / 4);
				memcpy(temp.data(), data.data(), data.size());
				fdata.resize(temp.size());
#pragma omp parallel for
				for (int i = 0; i < fdata.size(); i++)
					fdata[i] = temp[i];
				return;
			}
		}
		default:
			;
		}
	}

	int log_mel(std::vector<int8_t> &data, Matrix& mel) {

		set_data(data);
		if (fdata.empty()) return 0;

		// pre-emphasis 预加重 //高通滤波
		std::vector<float> emphasis_data;
		emphasis_data.resize(fdata.size());

#pragma omp parallel for
		for (int i = 1; i < fdata.size(); i++)
			emphasis_data[i] = fdata[i] - fdata[i - 1] * preemphasis;
		emphasis_data[0] = fdata[0];

		Matrix mag;
		MagnitudeSpectrogram(emphasis_data, mag, length_DFT, hop_length, win_length);

		// 生成梅尔谱图 mel spectrogram      
		if (mel_basis.empty()) {
			mel_spectrogram_create(nSamplesPerSec, length_DFT, number_filterbanks,mel_basis);
		}
		
		mel.reset_type(mel_basis.rows, mag.rows);
		//std::clock_t begin = clock();
		//mel_basis * mag
		//C←αAB + βC  (M, K) * ( K, N)
		cblas_sgemm(CblasRowMajor, //表示数组时行为主
			        CblasNoTrans, //A不转置
			        CblasTrans, //B转置
			        mel_basis.rows, //M 
			        mag.rows,  //N
			        mag.cols, //K
			        1, // alpha = 1 
			        mel_basis.data.data(),//A
			        mel_basis.cols, //A的列
			        mag.data.data(),//B
			        mag.cols,//B的列 
			        0, //beta = 0
			        mel.data.data(), //C 
			        mel.cols //C的列
		            );
		//printf("dot time is %d\n", clock() - begin);

#pragma omp parallel for
		for (int i = 0; i < mel.data.size(); i++) {
			mel.data[i] = log10(_max(mel.data[i], 1e-5f)) * 20;
			//mel.data[i] = (mel.data[i] - ref_db + max_db ) / max_db;
			//mel.data[i] = _max(_min(mel.data[i], 1.0f), 1e-8f);
		}
		return 1;
	}	

private:
	int nSamplesPerSec = 16000;
	int16_t byte_per_sample = 2;
	float preemphasis = 0.97;
	int length_DFT = 2048;
	int hop_length = 200;
	int win_length = 800;

	Matrix mel_basis;
	std::vector<float> hannWindow;

	int number_filterbanks = 80;
	int ref_db = 20;
	int max_db = 100;
	const float pi = 3.1415926535897;
	std::vector<float> fdata;

	inline float _max(float a, float b) { return a > b ? a : b; }
	inline float _min(float a, float b) { return a > b ? b : a; }	


	float hz_to_mel(float frequencies, bool htk = false) {
		if(htk) {
			return 2595.0 * log10(1.0 + frequencies / 700.0);
		}
		// Fill in the linear part
		float f_min = 0.0;
		float f_sp = 200.0 / 3;
		float mels = (frequencies - f_min) / f_sp;
		// Fill in the log-scale part
		float min_log_hz = 1000.0;                         // beginning of log region (Hz)
		float min_log_mel = (min_log_hz - f_min) / f_sp;   // same (Mels)
		float logstep = log(6.4) / 27.0;              // step size for log region

		if (frequencies >= min_log_hz) {
			// If we have scalar data, heck directly
			mels = min_log_mel + log(frequencies / min_log_hz) / logstep;
		}
		return mels;
	}

	//Convert mel bin numbers to frequencies
	int mel_to_hz(std::vector<float>& mels, std::vector<float>& freqs) {
		
		// Fill in the linear scale
		float f_min = 0.0;
		float f_sp = 200.0 / 3;

		freqs.resize(mels.size());

		// And now the nonlinear scale
		float min_log_hz = 1000.0;                         // beginning of log region (Hz)
		float min_log_mel = (min_log_hz - f_min) / f_sp;   // same (Mels)
		float logstep = log(6.4) / 27.0;              // step size for log region

#pragma omp parallel for
		for (int i = 0; i < mels.size(); i++) {
			if (mels[i] >= min_log_mel)
				freqs[i] = exp((mels[i] - min_log_mel) * logstep) * min_log_hz;
			else
				freqs[i] = mels[i] * f_sp + f_min;
		}		
		return 1;
	}

	// 生成等差数列，类似np.linspace
	int cvlinspace(float min_, float max_, int length, std::vector<float> &seq) {

		seq.resize(length);
#pragma omp parallel for
		for (int i = 0; i < length; i++) {
			seq[i] = ((max_ - min_) / (length - 1) * i) + min_;
		}
		return 1;
	}

	// Create a Filterbank matrix to combine FFT bins into Mel-frequency bins
	int mel_spectrogram_create(int nps, int n_fft, int n_mels, Matrix& weights) {

		float f_max = nps / 2.0;
		float f_min = 0;
		int n_fft_2 = 1 + n_fft / 2;

		weights.reset_type(n_mels, n_fft_2);

		// Center freqs of each FFT bin
		std::vector<float> fftfreqs;
		cvlinspace(f_min, f_max, n_fft_2, fftfreqs);

		// 'Center freqs' of mel bands - uniformly spaced between limits
		float min_mel = hz_to_mel(f_min, false);
		float max_mel = hz_to_mel(f_max, false);

		std::vector<float> mels;
		cvlinspace(min_mel, max_mel, n_mels + 2, mels);
	
		std::vector<float> mel_f;
		mel_to_hz(mels, mel_f);
	
		std::vector<float> fdiff(mel_f.size() - 1);
#pragma omp parallel for
		for (int i = 0; i < fdiff.size(); i++)
			fdiff[i] = mel_f[i + 1] - mel_f[i];

		Matrix ramps(mel_f.size(), fftfreqs.size());
#pragma omp parallel for
		for (int i = 0; i < ramps.data.size(); i++) {
			ramps.data[i] = mel_f[ramps.get_row(i)] - fftfreqs[ramps.get_col(i)];
		}
	
#pragma omp parallel for
		for (int i = 0; i < n_mels; i++) {
			// lower and upper slopes for all bins
			// Slaney-style mel is scaled to be approx constant energy per channel
			float enorm = 2.0 / (mel_f[i + 2] - mel_f[i]);
			for (int j = 0; j < weights.cols; j++) {
				float lower = ramps.data[ramps.get_index(i, j)] * -1 / fdiff[i];
				float upper = ramps.data[ramps.get_index(i + 2, j)] / fdiff[i + 1];
				weights.data[weights.get_index(i, j)] = _max((float)0, _min(lower, upper)) * enorm;
			}
		}
		return 1;
	}
	


	//Short-time Fourier transform (STFT): 默认center=True, window='hann', pad_mode='reflect'
	int MagnitudeSpectrogram(std::vector<float>& emphasis_data, 
		                                 Matrix&  feature_vector,
		                                 int n_fft = 2048, 
		                                 int hop_length = 512, 
		                                 int win_length = 2048) {
		
		// reflect对称填充，对应opencv的cv::BORDER_REFLECT_101
		int pad_lenght = n_fft / 2;
		std::vector<float> padbuffer;
		padbuffer.resize(emphasis_data.size() + n_fft);
#pragma omp parallel for
		for(int i = 0; i < padbuffer.size(); i++){
			if (i < pad_lenght)
				padbuffer[i] = emphasis_data[pad_lenght - i];
			else if (i < emphasis_data.size() + pad_lenght)
				padbuffer[i] = emphasis_data[i - pad_lenght];
			else
				padbuffer[i] = emphasis_data[emphasis_data.size() - (i - pad_lenght - emphasis_data.size() + 2) ];
		}

		if (hannWindow.empty()) {			
			hannWindow = std::vector<float>(n_fft, 0.0f);
			int insert_cnt = (n_fft - win_length) / 2;
			if (insert_cnt <= 0) return 0;

#pragma omp parallel for
			for (int k = 1; k <= win_length; k++) {
				hannWindow[k - 1 + insert_cnt] = float(0.5 * (1 - cos(2 * pi * k / (win_length + 1))));
			}
		}


		int size = padbuffer.size();
		int number_feature_vectors = (size - n_fft) / hop_length + 1;
		int number_coefficients = n_fft / 2 + 1;
		feature_vector.reset_type(number_feature_vectors, number_coefficients);

		audiofft::AudioFFT fft; //将FFT初始化放在循环外，可达到最优速度
		fft.init(size_t(n_fft));


		for (int i = 0; i <= size - n_fft; i += hop_length) {

			std::vector<float> frame(n_fft);
#pragma omp parallel for
			for (int j = 0; j < frame.size(); j++) {
				frame[j] = hannWindow[j] * padbuffer[i + j];
			}

			// 复数：Xrf实数，Xif虚数。
			std::vector<float> Xrf(n_fft);
			std::vector<float> Xif(n_fft);
			fft.fft(frame.data(), Xrf.data(), Xif.data());

#pragma omp parallel for
			for (int j = 0; j < feature_vector.cols; j++) {
				// 求模
				feature_vector.data[feature_vector.get_index(i / hop_length, j)] = sqrt(pow(Xrf[j], 2) + pow(Xif[j], 2));
			}
		}
		return 1;
	}	
};

