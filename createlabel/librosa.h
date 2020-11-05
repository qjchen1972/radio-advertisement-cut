#pragma once

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include<math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include<ctime>
//#include <cblas.h>
#include "AudioFFT.h"
#include "math_tool.h"
#include "matrix.h"

#ifdef __unix
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
#define SHRT_MAX __SHRT_MAX__
#define FLT_EPSILON __FLT_EPSILON__
#define sprintf_s  sprintf
#endif

#ifndef GTYPE
#define GTYPE  float
#endif

class Librosa{

public:
	Librosa(){}
	~Librosa(){}	

	void set_param(int32_t sample_rate=16000, int16_t bit_per_sample=16, GTYPE hop_time=0.01, GTYPE win_time=0.04) {
		m_sample_rate = sample_rate;
		m_byte_per_sample = bit_per_sample;
		m_hop_len = int(hop_time * m_sample_rate); //0.0125
		m_win_len = int(win_time * m_sample_rate); //0.05
	}	

	void amp_to_db(Matrix& m) {
		m.proc([&](GTYPE value) {
			return log10(std::max(value, (GTYPE)1e-5)) * 20;
		});
	}

	void db_to_amp(Matrix& m) {
		m.proc([&](GTYPE value) {
			return pow(10, value * (GTYPE)0.05);
		});
	}

	//Short-time Fourier transform (STFT): 默认center=True, window='hann', pad_mode='reflect'
	int stft(std::vector<GTYPE>& emphasis_data, 
		     Complex_Matrix &md, 
		     int n_fft=0, 
		     int hop_length=0,
		     int win_length=0) {

		if (!n_fft) n_fft = m_fft_len;
		if (!hop_length) hop_length = m_hop_len;
		if (!win_length) win_length = m_win_len;

		if (m_hannWindow.empty()) {
			if (!get_window(m_hannWindow, win_length, n_fft)) return 0;
		}

		std::vector<GTYPE> padbuffer;
		tool.pad_center(emphasis_data, n_fft, padbuffer);		
		
		audiofft::AudioFFT fft; //将FFT初始化放在循环外，可达到最优速度
		fft.init(size_t(n_fft));

		int number_feature_vectors = (padbuffer.size() - n_fft) / hop_length + 1;
		int number_coefficients = audiofft::AudioFFT::ComplexSize(size_t(n_fft)); // n_fft / 2 + 1
		md.reset_type(number_feature_vectors, number_coefficients, 0);
	
		Matrix frame(1, n_fft);
		// 复数：Xrf实数，Xif虚数。
		std::vector<GTYPE> Xrf(number_coefficients);
		std::vector<GTYPE> Xif(number_coefficients);

		for (int i = 0; i <= padbuffer.size() - n_fft; i += hop_length) {
			frame = m_hannWindow * (padbuffer.data() + i);
			fft.fft(frame.data.data(), Xrf.data(), Xif.data());
			md.real.data.insert(md.real.data.end(), Xrf.begin(), Xrf.end());
			md.imag.data.insert(md.imag.data.end(), Xif.begin(), Xif.end());
		}
		return 1;
	}

	//def istft(stft_matrix, hop_length = None, win_length = None, 
	//window = 'hann',center = True, dtype = np.GTYPE32, length = None)
	int istft(Complex_Matrix &md, std::vector<GTYPE>& tds, int hop_length=0, int win_length=0){
		
		int n_fft = 2 * (md.real.cols - 1);
		if (!win_length) win_length = m_win_len;
		if (!hop_length) hop_length = m_hop_len;
		
		if (m_hannWindow.empty()) {
			if (!get_window(m_hannWindow, win_length, n_fft)) return 0;
		}

		//int n_frames = re.rows;
		int expected_signal_len = n_fft + hop_length * (md.real.rows - 1);
		Matrix y(1, expected_signal_len, (GTYPE)0);

		audiofft::AudioFFT fft; //将FFT初始化放在循环外，可达到最优速度
		fft.init(size_t(n_fft));
	
		Matrix frame(1, n_fft);

		for (int i = 0; i < md.real.rows; i++) {		
			int pos = i * md.real.cols;
			fft.ifft(frame.data.data(), md.real.data.data() + pos , md.imag.data.data() + pos);
			frame = m_hannWindow * frame;
			y.add(i * hop_length, frame);
		}
		
		if(m_ifft_window_sum.empty())
			if (!norm_sumsquare(m_ifft_window_sum, md.real.rows, win_length, n_fft, hop_length)) return 0;

		tds.resize(expected_signal_len - n_fft);
#pragma omp parallel for
		for (int i = 0; i < m_ifft_window_sum.data.size(); i++) {
			if (i >= n_fft / 2 && i < (y.data.size() - n_fft / 2)) {
				if (m_ifft_window_sum.data[i] > (GTYPE)1.1754944e-38/*FLT_EPSILON*/)
					y.data[i] /= m_ifft_window_sum.data[i];
				tds[i - n_fft / 2] = y.data[i];
			}
		}
		return 1;
	}

	void fft(std::vector<GTYPE>& sdata, Complex_Matrix &md, int32_t n_fft) {

		audiofft::AudioFFT fft; 
		fft.init(size_t(n_fft));

		int number_feature_vectors = sdata.size()/ n_fft;
		int number_coefficients = audiofft::AudioFFT::ComplexSize(size_t(n_fft)); // n_fft / 2 + 1
		md.reset_type(number_feature_vectors, number_coefficients, 0);
		std::vector<GTYPE> Xrf(number_coefficients);
		std::vector<GTYPE> Xif(number_coefficients);


		for (int i = 0; i <= sdata.size() - n_fft; i += n_fft) {
			fft.fft(sdata.data() + i, Xrf.data(), Xif.data());
			md.real.data.insert(md.real.data.end(), Xrf.begin(), Xrf.end());
			md.imag.data.insert(md.imag.data.end(), Xif.begin(), Xif.end());
		}
	}

	void ifft(Complex_Matrix &md, std::vector<GTYPE>& sdata, int32_t n_fft) {

		audiofft::AudioFFT fft;
		fft.init(size_t(n_fft));		
		sdata.resize(n_fft * md.real.rows);
		for (int i = 0; i < md.real.rows; i++) {
			int pos = i * md.real.cols;
			fft.ifft(sdata.data() + i * n_fft, md.real.data.data() + pos, md.imag.data.data() + pos);
		}
	}

	int mel_spectrogram(std::vector<int8_t> &data, Matrix& mel) {

		std::vector<GTYPE> fdata;
		tool.char2GTYPE(data, fdata, m_byte_per_sample);
		tool.wav_norm(fdata);

		std::vector<GTYPE> emphasis_data;
		tool.pre_emphasis(fdata, emphasis_data);

		Complex_Matrix md;
		if (!stft(emphasis_data, md)) return 0;

		Matrix mag = md.abs();
		//m_ph = md;
	
		//mag.transpos().show("mag:");

		// 生成梅尔谱图 mel spectrogram      
		if (!linear_to_mel(mag, mel)) return 0;	

		//mel.show("mel:");

		amp_to_db(mel);
		//return 1;
		//mel.show("db:");

		//得到包络
		mel = ((mel + (-m_ref_db)) + m_max_db) * (1/m_max_db);
		mel.proc([&](GTYPE value) {
			return std::max(std::min(value, (GTYPE)1.0), (GTYPE)1e-8);
		});
		return 1;
	}	

	int inv_mel_spectrogram(Matrix& mel, std::vector<int8_t> &wav, GTYPE power=1.0) {
		
		mel.proc([&](GTYPE value) {
			return std::max(std::min(value, (GTYPE)1.0), (GTYPE)1e-8);
		});
		mel = mel * m_max_db + (-m_max_db) + m_ref_db;
		db_to_amp(mel);

		//mel.show("db:");

		Matrix mag;
		if (!mel_to_linear(mel, mag)) return 0;

		//mag.show("mag0");

		mag.proc([&](GTYPE value) {
			return  pow(value, power);
		});
		
		//mag.show("mag1");
		mag = mag.transpos();
		

		std::vector<GTYPE> fdata, inv_data;
		//test
		//Complex_Matrix md = m_ph.norm() * mag;
		//if (!istft(md, fdata)) return 0;
		if (!griffin_lim(mag, fdata)) return 0;
		tool.inv_pre_emphasis(fdata, inv_data);

		tool.wav_denorm(inv_data);
		tool.GTYPE2char(inv_data, wav, m_byte_per_sample);
		return 1;
	}

	int compose_wav(Matrix& mag, Complex_Matrix &phrase, std::vector<int8_t> &wav) {
		Complex_Matrix md = phrase * mag;
		std::vector<GTYPE> fdata;
		if (!istft(md, fdata)) return 0;
		tool.GTYPE2char(fdata, wav, m_byte_per_sample);
		return 1;
	}	

private:
	//常量
	//const GTYPE m_pre_emphasis = 0.97;
	const int32_t m_filterbanks_num = 80;
	const GTYPE  m_ref_db = (GTYPE)20;
	const GTYPE  m_max_db = (GTYPE)100;
	const int32_t m_fft_len = 2048; //2的次方

	//一些需要设置的变量
	int32_t m_sample_rate = 16000;
	int16_t m_byte_per_sample = 16;
	int32_t m_hop_len = 200;
	int32_t m_win_len = 800;

	//设置一次，不需要变化的变量
	Matrix m_mel_basis;
	Matrix m_hannWindow;
	Matrix m_ifft_window_sum;

	Math_tool tool;

	//test
	Complex_Matrix m_ph;

	//生成梅尔谱图 mel spectrogram 
	int linear_to_mel(Matrix& mag, Matrix& mel) {

		if (m_mel_basis.empty()) {
			if (!mel_spectrogram_create(m_sample_rate, m_fft_len, m_filterbanks_num, m_mel_basis))
				return 0;
		}
		//m_mel_basis.show("mel");

		mel.reset_type(m_mel_basis.rows, mag.rows);
		std::clock_t begin = clock();
		//mel_basis * mag
		rowMatrixf A = Eigen::Map<rowMatrixf>(m_mel_basis.data.data(), m_mel_basis.rows, m_mel_basis.cols);
		rowMatrixf B = Eigen::Map<rowMatrixf>(mag.data.data(), mag.rows, mag.cols);
		rowMatrixf C;
		C = A * B.transpose();
		Eigen::Map<rowMatrixf>(mel.data.data(), mel.rows, mel.cols) = C;
		//std::cout << C << std::endl;
	
		//C←αAB + βC  (M, K) * ( K, N)
		/*cblas_sgemm(CblasRowMajor, //表示数组时行为主
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
		);*/
		printf("dot time is %d\n", clock() - begin);
		return 1;
	}
	
	int mel_to_linear(Matrix& mel, Matrix& mag) {
		if (m_mel_basis.empty()) {
			if (!mel_spectrogram_create(m_sample_rate, m_fft_len, m_filterbanks_num, m_mel_basis))
				return 0;
		}

		mag.reset_type(m_mel_basis.cols, mel.cols);

		rowMatrixf A = Eigen::Map<rowMatrixf>(m_mel_basis.data.data(), m_mel_basis.rows, m_mel_basis.cols);
		rowMatrixf B = Eigen::Map<rowMatrixf>(mel.data.data(), mel.rows, mel.cols);
		rowMatrixf C = tool.pinv(A) * B;		
		Eigen::Map<rowMatrixf>(mag.data.data(), mag.rows, mag.cols) = C;

		mag.proc([&](GTYPE value) {
			return std::max(value, (GTYPE)1e-10);
		});
		return 1;
	}		

	//Approximate magnitude spectrogram inversion using the "fast" Griffin-Lim algorithm
	int griffin_lim(Matrix& mag, std::vector<GTYPE>& fdata, int n_iter=1, GTYPE momentum=0.99) {
	
		Matrix angle(mag.rows, mag.cols);
		angle.random((GTYPE)0, (GTYPE)1);
		Complex_Matrix phrase;
		phrase.polar(angle);

		Complex_Matrix rebuilt(mag.rows, mag.cols, (GTYPE)0.0);
		Complex_Matrix tprev;

		for (int i = 0; i < n_iter; i++) {
			tprev = rebuilt;
			Complex_Matrix src = phrase * mag;
			if (!istft(src, fdata)) return 0;
			if (i == n_iter - 1) return 1;
			if (!stft(fdata, rebuilt)) return 0;
			phrase = rebuilt + tprev * ( -momentum / (1 + momentum));
			phrase = phrase.norm();
		}
		return 1;
	}

	//get hann window
	int get_window(Matrix &win, int win_length, int n_fft, int squ = 0) {

		win = Matrix(1, n_fft, (GTYPE)0.0);
		int insert_cnt = (n_fft - win_length) / 2;
		if (insert_cnt < 0) return 0;

		double value;
#pragma omp parallel for
		for (int k = 1; k <= win_length; k++) {
			value = 0.5 * (1 - cos(2 * M_PI * k / (win_length + 1)));
			if (squ) value *= value;
			win.data[k - 1 + insert_cnt] = (GTYPE)value;
		}
		return 1;
	}

	//Normalize by sum of squared window
	int norm_sumsquare(Matrix& y, int n_frames, int win_length, int n_fft, int hop_length) {

		int n = n_fft + hop_length * (n_frames - 1);
		y = Matrix(1, n, (GTYPE)0);

		Matrix squareWindow;
		if (!get_window(squareWindow, win_length, n_fft, 1)) return 0;

		for (int i = 0; i < n_frames; i++)
			y.add(i * hop_length, squareWindow);
		return 1;
	}


	GTYPE hz_to_mel(GTYPE frequencies, bool htk = false) {
		if(htk) {
			return 2595.0 * log10(1.0 + frequencies / 700.0);
		}
		// Fill in the linear part
		GTYPE f_min = 0.0;
		GTYPE f_sp = 200.0 / 3;
		GTYPE mels = (frequencies - f_min) / f_sp;
		// Fill in the log-scale part
		GTYPE min_log_hz = 1000.0;                         // beginning of log region (Hz)
		GTYPE min_log_mel = (min_log_hz - f_min) / f_sp;   // same (Mels)
		GTYPE logstep = log(6.4) / 27.0;              // step size for log region

		if (frequencies >= min_log_hz) {
			// If we have scalar data, heck directly
			mels = min_log_mel + log(frequencies / min_log_hz) / logstep;
		}
		return mels;
	}

	//Convert mel bin numbers to frequencies
	int mel_to_hz(std::vector<GTYPE>& mels, std::vector<GTYPE>& freqs) {
		
		// Fill in the linear scale
		GTYPE f_min = 0.0;
		GTYPE f_sp = 200.0 / 3;
		freqs.resize(mels.size());

		// And now the nonlinear scale
		GTYPE min_log_hz = 1000.0;                         // beginning of log region (Hz)
		GTYPE min_log_mel = (min_log_hz - f_min) / f_sp;   // same (Mels)
		GTYPE logstep = log(6.4) / 27.0;              // step size for log region

#pragma omp parallel for
		for (int i = 0; i < mels.size(); i++) {
			if (mels[i] >= min_log_mel)
				freqs[i] = exp((mels[i] - min_log_mel) * logstep) * min_log_hz;
			else
				freqs[i] = mels[i] * f_sp + f_min;
		}		
		return 1;
	}	

	// Create a Filterbank matrix to combine FFT bins into Mel-frequency bins
	int mel_spectrogram_create(int nps, int n_fft, int n_mels, Matrix& weights) {

		GTYPE f_max = nps / 2.0;
		GTYPE f_min = 0;
		int n_fft_2 = 1 + n_fft / 2;

		weights.reset_type(n_mels, n_fft_2);

		// Center freqs of each FFT bin
		std::vector<GTYPE> fftfreqs;
		tool.cvlinspace(f_min, f_max, n_fft_2, fftfreqs);

		// 'Center freqs' of mel bands - uniformly spaced between limits
		GTYPE min_mel = hz_to_mel(f_min, false);
		GTYPE max_mel = hz_to_mel(f_max, false);

		std::vector<GTYPE> mels;
		tool.cvlinspace(min_mel, max_mel, n_mels + 2, mels);
	
		std::vector<GTYPE> mel_f;
		mel_to_hz(mels, mel_f);
	
		std::vector<GTYPE> fdiff(mel_f.size() - 1);
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
			GTYPE enorm = 2.0 / (mel_f[i + 2] - mel_f[i]);
			for (int j = 0; j < weights.cols; j++) {
				GTYPE lower = ramps.data[ramps.get_index(i, j)] * -1 / fdiff[i];
				GTYPE upper = ramps.data[ramps.get_index(i + 2, j)] / fdiff[i + 1];
				weights.data[weights.get_index(i, j)] = std::max((GTYPE)0, std::min(lower, upper)) * enorm;
			}
		}
		return 1;
	}
};

