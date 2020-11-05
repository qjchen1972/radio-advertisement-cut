#pragma once
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif 
#include<math.h>
#include<algorithm>
#include<vector>
//#define EIGEN_USE_MKL_ALL
//#define EIGEN_USE_BLAS 
#include <Eigen/Dense>
#include <Eigen/SVD>

#ifndef GTYPE
#define GTYPE  float
#endif

typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> rowMatrixf;
class Math_tool {

public:
	Math_tool() {}
	~Math_tool(){}

	void show(std::vector<GTYPE> list, int line) {
		printf(" %d\n", list.size());
		for (int i = 0; i < list.size(); i++) {
			if (i % line == 0) printf("\n");
			printf("%8.3f ", list[i]);
		}
		printf("\n");
	}

	//广义逆矩阵
	rowMatrixf pinv(rowMatrixf  A)
	{
		Eigen::JacobiSVD<rowMatrixf> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);//M=USV*
		//Eigen::JacobiSVD<Eigen::MatrixXf> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
		GTYPE  pinvtoler = (GTYPE)1.e-8; //tolerance
		int row = A.rows();
		int col = A.cols();
		int k = std::min(row, col);
		rowMatrixf X = rowMatrixf::Zero(col, row);
		rowMatrixf singularValues_inv = svd.singularValues();//奇异值
		rowMatrixf singularValues_inv_mat = rowMatrixf::Zero(col, row);

#pragma omp parallel for
		for (long i = 0; i < k; ++i) {
			if (singularValues_inv(i) > pinvtoler)
				singularValues_inv(i) = 1.0 / singularValues_inv(i);
			else singularValues_inv(i) = 0;
			singularValues_inv_mat(i, i) = singularValues_inv(i);
		}
		X = (svd.matrixV())*(singularValues_inv_mat)*(svd.matrixU().transpose());//X=VS+U*
		return X;
	}

	// 取得均匀分布的随机数
	GTYPE uniform_rand(GTYPE dMinValue, GTYPE dMaxValue) {
		GTYPE pRandomValue = (GTYPE)rand() / RAND_MAX;
		pRandomValue = pRandomValue * (dMaxValue - dMinValue) + dMinValue;
		return pRandomValue;
	}

	//从src随机取dstlen个
	void rand_list(GTYPE* src, int srclen, GTYPE* dst, int dstlen) {
		std::vector<int> index(srclen);
		for (int i = 0; i < srclen; i++)
			index[i] = i;
		srand(time(0));
		std::random_shuffle(index.begin(), index.end());

		std::vector<int> dst_index;
		dst_index.insert(dst_index.end(), index.begin(), index.begin() + dstlen);
		std::sort(dst_index.begin(), dst_index.end());	
		for (int i = 0; i < dstlen; i++)
			dst[i] = src[dst_index[i]];		
	}

	// 生成等差数列，类似np.linspace
	void cvlinspace(GTYPE min_, GTYPE max_, int length, std::vector<GTYPE> &seq) {

		seq.resize(length);
#pragma omp parallel for
		for (int i = 0; i < length; i++) {
			seq[i] = ((max_ - min_) / (length - 1) * i) + min_;
		}
	}
	
	// reflect对称填充，对应opencv的cv::BORDER_REFLECT_101
	void pad_center(std::vector<GTYPE> &data, int n_fft, std::vector<GTYPE> &padbuffer) {

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


	//声音归一到[-1,1]
	void wav_norm(std::vector<GTYPE> &fdata) {
		GTYPE m = fabs(fdata[0]);
		for (int i = 1; i < fdata.size(); i++)
			if (fabs(fdata[i]) > m) m = fabs(fdata[i]);
#pragma omp parallel for
		for (int i = 0; i < fdata.size(); i++)
			fdata[i] /= std::max((GTYPE)0.01, m);
	}

	//从[-1,1] 回到[-max,max]
	void wav_denorm(std::vector<GTYPE> &fdata, GTYPE max=32767) {
		GTYPE m = fabs(fdata[0]);
		for (int i = 1; i < fdata.size(); i++)
			if (fabs(fdata[i]) > m) m = fabs(fdata[i]);
#pragma omp parallel for
		for (int i = 0; i < fdata.size(); i++)
			fdata[i] *= max / std::max((GTYPE)0.01, m);
	}

	//data 的item_size组成一个数，赋值给fdata
	void char2GTYPE(std::vector<int8_t> &data, std::vector<GTYPE> &fdata, int item_size) {

		int size = item_size / 8;
		fdata.resize(data.size() / size);

#pragma omp parallel for
		for (int i = 0; i < fdata.size(); i++) {
			if (size == 1)
				fdata[i] = (GTYPE)*((int8_t*)(data.data() + size * i));
			else if (size == 2)
				fdata[i] = (GTYPE)*((int16_t*)(data.data() + size * i));
			else
				fdata[i] = (GTYPE)*((int32_t*)(data.data() + size * i));
		}
	}

	void GTYPE2char(std::vector<GTYPE> &fdata, std::vector<int8_t> &data, int item_size) {

		int size = item_size / 8;
		data.resize(fdata.size() * size);

#pragma omp parallel for
		for (int i = 0; i < data.size(); i += size) {
			if (size == 1)
				data[i] = (int8_t)*((GTYPE*)(fdata.data() + i / size));
			else if (size == 2) {
				int16_t temp = (int16_t)*((GTYPE*)(fdata.data() + i / size));
				memcpy(&data[i], &temp, sizeof(int16_t));
			}
			else {
				int32_t temp = (int32_t)*((GTYPE*)(fdata.data() + i / size));
				memcpy(&data[i], &temp, sizeof(int32_t));
			}
		}
	}

	// pre_emphasis 预加重 ,高通滤波
	void pre_emphasis(std::vector<GTYPE>& fdata, std::vector<GTYPE>& emphasis_data, GTYPE pre_emphasis=0.97) {

		emphasis_data.resize(fdata.size());
#pragma omp parallel for
		for (int i = 1; i < fdata.size(); i++)
			emphasis_data[i] = fdata[i] - fdata[i - 1] * pre_emphasis;
		emphasis_data[0] = fdata[0];
	}


	//inv_preemphasis 逆预加重
	void inv_pre_emphasis(std::vector<GTYPE>& fdata, 
		                  std::vector<GTYPE>& inv_emphasis_data, 
		                  GTYPE pre_emphasis=0.97) {
		inv_emphasis_data.resize(fdata.size());
		inv_emphasis_data[0] = 0;
#pragma omp parallel for
		for (int i = 1; i < fdata.size(); i++)
			inv_emphasis_data[i] = fdata[i] + inv_emphasis_data[i - 1] * pre_emphasis;
	}

};