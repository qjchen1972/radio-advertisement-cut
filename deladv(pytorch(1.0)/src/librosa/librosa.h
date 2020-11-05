#pragma once
#include<math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cblas.h>
#include "AudioFFT.h"
#include "matrix.h"

#ifdef __unix
#define FLT_EPSILON __FLT_EPSILON__
#endif

void show(std::vector<float> list, int line) {
	printf("\n");
	for (int i = 0; i < list.size(); i++) {
		if (i % line == 0) printf("\n");
		printf("%d ", list[i]);
	}
	printf("\n");
}

class Librosa {

public:
	Librosa(){}
	~Librosa(){}	

	void set_param(int32_t sample_rate = 16000, int16_t bit_per_sample = 16) {
		nSamplesPerSec = sample_rate;
		byte_per_sample = bit_per_sample;
		hop_length = int(0.0125 * nSamplesPerSec); //0.0125
		win_length = int(0.05 * nSamplesPerSec); //0.05
		length_DFT = 2048;
	}	

	//Short-time Fourier transform (STFT): 默认center=True, window='hann', pad_mode='reflect'
	int stft(std::vector<float>& emphasis_data,
		     Matrix& re,
		     Matrix& im,
		     int n_fft = 2048,
		     int hop_length = 512,
		     int win_length = 2048) {

		if (hannWindow.empty()) {
			if (!get_window(hannWindow, win_length, n_fft)) return 0;
		}

		std::vector<float> padbuffer;
		pad_center(emphasis_data, n_fft, padbuffer);		
	
	
		audiofft::AudioFFT fft; //将FFT初始化放在循环外，可达到最优速度
		fft.init(size_t(n_fft));

		int number_feature_vectors = (padbuffer.size() - n_fft) / hop_length + 1;
		int number_coefficients = audiofft::AudioFFT::ComplexSize(size_t(n_fft)); // n_fft / 2 + 1
		re.reset_type(number_feature_vectors, number_coefficients, 0);
		im.reset_type(number_feature_vectors, number_coefficients, 0);

		for (int i = 0; i <= padbuffer.size() - n_fft; i += hop_length) {

			std::vector<float> frame(n_fft);
#pragma omp parallel for
			for (int j = 0; j < frame.size(); j++) {
				frame[j] = hannWindow[j] * padbuffer[i + j];
			}
			// 复数：Xrf实数，Xif虚数。
			std::vector<float> Xrf(number_coefficients);
			std::vector<float> Xif(number_coefficients);
			fft.fft(frame.data(), Xrf.data(), Xif.data());
			re.data.insert(re.data.end(), Xrf.begin(), Xrf.end());
			im.data.insert(im.data.end(), Xif.begin(), Xif.end());
		}
		return 1;
	}

	//def istft(stft_matrix, hop_length = None, win_length = None, 
	//window = 'hann',center = True, dtype = np.float32, length = None)
	int istft(Matrix&  re, Matrix& im, std::vector<float>& tds, int hop_length=0, int win_length=0){

		int n_fft = 2 * (re.cols - 1);
		if (!win_length) win_length = n_fft;
		if (!hop_length) hop_length = win_length / 4;
		if (hannWindow.empty()) {
			if (!get_window(hannWindow, win_length, n_fft)) return 0;
		}
		int n_frames = re.rows;
		int expected_signal_len = n_fft + hop_length * (n_frames - 1);
		std::vector<float> y(expected_signal_len, 0.0f);

		audiofft::AudioFFT fft; //将FFT初始化放在循环外，可达到最优速度
		fft.init(size_t(n_fft));
		tds.resize(expected_signal_len - n_fft);

		for (int i = 0; i < re.rows; i++) {
			std::vector<float> frame(n_fft);
			std::vector<float> Xrf(re.cols);
			std::vector<float> Xif(re.cols);
			int start = i * re.cols * sizeof(float);
			memcpy(Xrf.data(), re.data.data() + start, re.cols * sizeof(float));
			memcpy(Xif.data(), im.data.data() + start, im.cols * sizeof(float));
			fft.ifft(frame.data(), Xrf.data(), Xif.data());
			//#pragma omp parallel for
			for (int j = 0; j < frame.size(); j++) {
				frame[j] = hannWindow[j] * frame[j];
				y[i * hop_length + j] += frame[j];
			}
		}

		std::vector<float> ifft_window_sum;
		if (!norm_sumsquare(ifft_window_sum, n_frames, win_length, n_fft, hop_length)) return 0;
#pragma omp parallel for
		for (int i = 0; i < ifft_window_sum.size(); i++) {
			if (ifft_window_sum[i] > FLT_EPSILON)
				y[i] /= ifft_window_sum[i];
			if (i >= n_fft / 2 && i < (y.size() - n_fft / 2))
				tds[i] = y[i];
		}
		return 1;
	}

	//求模
	void magnitude(Matrix& re, Matrix& im, Matrix& mag) {
		mag.reset_type(re);
#pragma omp parallel for
		for (int i = 0; i < mag.data.size(); i++) {
			mag.data[i] = sqrt(pow(re.data[i], 2) + pow(im.data[i], 2));
		}
	}

	//求弧度表达相位
	void phrase(Matrix& re, Matrix& im, Matrix& angle) {
		angle.reset_type(re);
#pragma omp parallel for
		for (int i = 0; i < angle.data.size(); i++) {
			angle.data[i] = atan2(im.data[i], re.data[i]);
		}
	}

	//求虚数表达相位
	void phrase(Matrix& re, Matrix& im, Matrix& ph_re, Matrix& ph_im) {

		ph_re.reset_type(re);
		ph_im.reset_type(re);

#pragma omp parallel for
		for (int i = 0; i < re.data.size(); i++) {
			float mag = sqrt(pow(re.data[i], 2) + pow(im.data[i], 2));
			ph_re.data[i] = re.data[i] / mag;
			ph_im.data[i] = im.data[i] / mag;
		}
	}

	// 计算频域 = 幅度 * 相位
	void get_tds(Matrix& mag, Matrix& ph_re, Matrix& ph_im, Matrix& re, Matrix& im) {
		re.reset_type(mag);
		im.reset_type(mag);

#pragma omp parallel for
		for (int i = 0; i < mag.data.size(); i++) {
			re.data[i] = ph_re.data[i] * mag.data[i];
			im.data[i] = ph_im.data[i] * mag.data[i];
		}
	}

	int log_mel(std::vector<int8_t> &data, Matrix& mel) {

		std::vector<float> fdata;
		char2float(data, fdata);

		printf("%d, %d, %d ,%d \n", fdata.size(), length_DFT, hop_length, win_length);

		std::vector<float> emphasis_data;
		pre_emphasis(fdata, emphasis_data);

		Matrix re;
		Matrix im;

		show(emphasis_data, 128);

		if (!stft(emphasis_data, re, im, length_DFT, hop_length, win_length)) return 0;

		std::vector<float> tds;
		istft(re, im, tds, hop_length, win_length);
		show(tds, 128);
		//Matrix re1 = re;
		//Matrix im1 = im;
		//re1.transpos();
		//im1.transpos();
		//re1.show("stft re:");
		//im1.show("stft im:");

		Matrix mag;
		magnitude(re, im, mag);
		//Matrix mag1 = mag;
		//mag1.transpos();
		//mag1.show("mag:");
		//MagnitudeSpectrogram(emphasis_data, mag, length_DFT, hop_length, win_length);

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
			//mel.data[i] = mel.data[i] / max_db;
			//mel.data[i] = _max(_min(mel.data[i], 1.0f), 1e-8f);
		}
		return 1;
	}	

private:
	int nSamplesPerSec = 16000;
	int16_t byte_per_sample = 16;
	float preemphasis = 0.97;
	int length_DFT = 2048;
	int hop_length = 200;
	int win_length = 800;

	Matrix mel_basis;
	std::vector<float> hannWindow;
	std::vector<float> squareWindow;

	int number_filterbanks = 80;
	int ref_db = 20;
	int max_db = 100;
	const float pi = 3.1415926535897;
	
	inline float _max(float a, float b) { return a > b ? a : b; }
	inline float _min(float a, float b) { return a > b ? b : a; }	

	void char2float(std::vector<int8_t> &data, std::vector<float> &fdata) {

		int size = byte_per_sample / 8;
		fdata.resize(data.size() / size);

#pragma omp parallel for
		for (int i = 0; i < fdata.size(); i++) {
			if (size == 1)
				fdata[i] = (int)*((int8_t*)(data.data() + size * i));
			else if (size == 2)
				fdata[i] = (int)*((int16_t*)(data.data() + size * i));
			else
				fdata[i] = (int)*((int32_t*)(data.data() + size * i));
		}
	}

	// pre_emphasis 预加重 ,高通滤波
	void pre_emphasis(std::vector<float>& fdata, std::vector<float>& emphasis_data) {

		emphasis_data.resize(fdata.size());
#pragma omp parallel for
		for (int i = 1; i < fdata.size(); i++)
			emphasis_data[i] = fdata[i] - fdata[i - 1] * preemphasis;
		emphasis_data[0] = fdata[0];
	}


	//inv_preemphasis 逆预加重
	void inv_pre_emphasis(std::vector<float>& fdata, std::vector<float>& inv_emphasis_data) {
		inv_emphasis_data.resize(fdata.size());
		inv_emphasis_data[0] = 0;
#pragma omp parallel for
		for (int i = 1; i < fdata.size(); i++)
			inv_emphasis_data[i] = fdata[i] + inv_emphasis_data[i - 1] * preemphasis;	
	}

	// reflect对称填充，对应opencv的cv::BORDER_REFLECT_101
	void pad_center(std::vector<float> &data, int n_fft, std::vector<float> &padbuffer) {

		int pad_lenght = n_fft / 2;
		padbuffer.resize(data.size() + n_fft);

#pragma omp parallel for
		for (int i = 0; i < padbuffer.size(); i++) {
			if (i < pad_lenght)
				padbuffer[i] = data[pad_lenght - i];
			else if (i < data.size() + pad_lenght)
				padbuffer[i] = data[i - pad_lenght];
			else
				padbuffer[i] = data[data.size() - (i - pad_lenght - data.size() + 2)];
		}
	}

	


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
			if (!get_window(hannWindow, win_length, n_fft)) return 0;
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

	//get hann window
	int get_window(std::vector<float> &win, int win_length, int n_fft, int squ=0) {

		win = std::vector<float>(n_fft, 0.0f);
		int insert_cnt = (n_fft - win_length) / 2;
		if (insert_cnt <= 0) return 0;

		double value;
#pragma omp parallel for
		for (int k = 1; k <= win_length; k++) {
			value = 0.5 * (1 - cos(2 * pi * k / (win_length + 1)));
			if (squ) value *= value;
			win[k - 1 + insert_cnt] = (float)value;
		}
		return 1;
	}

	// Fill the envelope
	void window_ss_fill(std::vector<float>& x, std::vector<float>& win_sq, int hop_length) {

		for (int i = 0; i < x.size(); i += hop_length) {
			for (int j = 0; j < win_sq.size(); j++) {
				if (i + j >= x.size()) break;
				x[i + j] += win_sq[j];
			}
		}
	}

	//Normalize by sum of squared window
	int norm_sumsquare(std::vector<float>& y, int n_frames, int win_length, int n_fft, int hop_length) {

		int n = n_fft + hop_length * (n_frames - 1);
		y = std::vector<float>(n, 0.0f);

		if (squareWindow.empty()) {
			if (!get_window(squareWindow, win_length, n_fft, 1)) return 0;
		}
		window_ss_fill(y, squareWindow, hop_length);
		return 1;
	}	
};

