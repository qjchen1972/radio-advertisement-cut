#pragma once
#include<vector>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include<math.h>

#ifndef GTYPE
#define GTYPE  float
#endif

class Matrix {
public:
	Matrix() {}
	Matrix(int32_t row, int32_t col) {
		rows = row;
		cols = col;
		data.resize(row*col);	
	}

	Matrix(int32_t row, int32_t col, GTYPE value) {
		rows = row;
		cols = col;
		data = std::vector<GTYPE>(row * col, value);
	}	

	Matrix(const Matrix& cls) {
		rows = cls.rows;
		cols = cls.cols;
		data.resize(0);
		data.insert(data.end(), cls.data.begin(), cls.data.end());
	}

	Matrix& operator=(const Matrix& cls) {

		if (this != &cls) {
			rows = cls.rows;
			cols = cls.cols;
			data.resize(0);
			data.insert(data.end(), cls.data.begin(), cls.data.end());
		}
		return *this;
	}

	inline bool empty() { return rows == 0; }

	inline void reset_type(const Matrix& cls) {
		rows = cls.rows;
		cols = cls.cols;
		data.resize(rows * cols);
	}

	inline void reset_type(int32_t row, int32_t col, int size = -1) {
		rows = row;
		cols = col;
		if (size == -1)
			data.resize(rows * cols);
		else
			data.resize(size);
	}

	inline int32_t get_row(int32_t index) {
		return index / cols;
	}

	inline int32_t get_col(int32_t index) {
		return index % cols;
	}

	inline int32_t get_index(int32_t r, int32_t c) {
		return r * cols + c;
	}

	Matrix transpos() {

		Matrix temp(cols, rows);
	#pragma omp parallel for
		for (int i = 0; i < data.size(); i++) {
			temp.data[i] = data[get_index(temp.get_col(i), temp.get_row(i))];
		}
		return temp;
	}

	void random(GTYPE min, GTYPE max) {
		
		srand(time(NULL));
		Math_tool tool;
#pragma omp parallel for
		for (int i = 0; i < data.size(); i++)
			data[i] = tool.uniform_rand(min, max);
	}

	void  proc(std::function<GTYPE (GTYPE)> fun) {
#pragma omp parallel for
		for (int i = 0; i < data.size(); i++) {
			data[i] = fun(data[i]);
		}
	}


	Matrix operator+ (Matrix m) {
		Matrix ret;
		ret.reset_type(*this);

#pragma omp parallel for
		for (int i = 0; i < ret.data.size(); i++) {
			ret.data[i] = this->data[i] + m.data[i];
		}
		return ret;
	}

	Matrix operator+ (const GTYPE *buf) {
		Matrix ret;
		ret.reset_type(*this);

#pragma omp parallel for
		for (int i = 0; i < ret.data.size(); i++) {
			ret.data[i] = this->data[i] + buf[i];
		}
		return ret;
	}

	Matrix operator+ (GTYPE value) {
		Matrix ret;
		ret.reset_type(*this);

#pragma omp parallel for
		for (int i = 0; i < ret.data.size(); i++) {
			ret.data[i] = this->data[i] + value;
		}
		return ret;
	}

	Matrix operator* (Matrix m) {
		Matrix ret;
		ret.reset_type(*this);

#pragma omp parallel for
		for (int i = 0; i < ret.data.size(); i++) {
			ret.data[i] = this->data[i] * m.data[i];
		}
		return ret;
	}

	Matrix operator* (const GTYPE *buf) {
		Matrix ret;
		ret.reset_type(*this);

#pragma omp parallel for
		for (int i = 0; i < ret.data.size(); i++) {
			ret.data[i] = this->data[i] * buf[i];
		}
		return ret;
	}

	Matrix operator* (GTYPE value) {
		Matrix ret;
		ret.reset_type(*this);

#pragma omp parallel for
		for (int i = 0; i < ret.data.size(); i++) {
			ret.data[i] = this->data[i] * value;
		}
		return ret;
	}

	Matrix operator/ (Matrix m) {
		Matrix ret;
		ret.reset_type(*this);

#pragma omp parallel for
		for (int i = 0; i < ret.data.size(); i++) {
			ret.data[i] = this->data[i]/ (m.data[i] + (GTYPE)1e-16);
		}
		return ret;
	}

	void add(int start, Matrix m) {

#pragma omp parallel for
		for (int i = 0; i < m.data.size(); i++) {
			if (start + i >= this->data.size()) return;
			this->data[start + i] += m.data[i];
		}
	}

	void mul(int start, Matrix m) {

#pragma omp parallel for
		for (int i = 0; i < m.data.size(); i++) {
			if (start + i >= this->data.size()) return;
			this->data[start + i] *= m.data[i];
		}
	}

	void show(const char* title) {
		printf("%s (%d, %d)\n", title, rows, cols);
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++)
				printf("%9.5f,", data[i*cols + j]);
			printf("\n");
		}
		return;
	}

	~Matrix() {}	

	std::vector<GTYPE> data;
	int32_t rows = 0;
	int32_t cols = 0;
private:
};


class  Complex_Matrix{
public:
	Complex_Matrix(){}
	Complex_Matrix(Matrix re, Matrix im):real(re),imag(im) {}

	Complex_Matrix(int32_t row, int32_t col) {
		real.reset_type(row, col);
		imag.reset_type(row, col);
	}

	Complex_Matrix(int32_t row, int32_t col, GTYPE value) {
		real = Matrix(row, col, value);
		imag = Matrix(row, col, value);
	}

	Complex_Matrix(const Complex_Matrix& cls) {
		real = cls.real;
		imag = cls.imag;
	}

	Complex_Matrix& operator=(const Complex_Matrix& cls) {
		if (this != &cls) {
			real = cls.real;
			imag = cls.imag;
		}
		return *this;
	}

	void reset_type(int32_t row, int32_t col, int size = -1) {
		real.reset_type(row, col, size);
		imag.reset_type(row, col, size);
	}

	~Complex_Matrix() {}

	//ÇóÄ£
	Matrix abs( ) {
		Matrix m;
		m.reset_type(real);
#pragma omp parallel for
		for (int i = 0; i < m.data.size(); i++) {
			m.data[i] = sqrt(pow(real.data[i], 2) + pow(imag.data[i], 2));
		}
		return m;
	}

	Matrix arg() {
		Matrix m;
		m.reset_type(real);
#pragma omp parallel for
		for (int i = 0; i < m.data.size(); i++) {
			m.data[i] = atan2(imag.data[i], real.data[i]); 
		}
		return m;
	}

	Complex_Matrix norm() {
		Complex_Matrix m;
		m.reset_type(real.rows, real.cols);

		Matrix mag = abs();
		m.real = real / mag;
		m.imag = imag / mag;
		return m;
	}

	void polar(const Matrix& m) {

		this->reset_type(m.rows, m.cols);
#pragma omp parallel for
		for (int i = 0; i < m.data.size(); i++) {
			GTYPE phrase = 2 * M_PI * m.data[i];
			real.data[i] = cos(phrase);
			imag.data[i] = sin(phrase);
		}
	}

	
	Complex_Matrix operator+ (const Complex_Matrix& cm) {

		Complex_Matrix ret;
		ret.real = this->real + cm.real;
		ret.imag = this->imag + cm.imag;
		return ret;
	}

	Complex_Matrix operator* (const Matrix& m) {

		Complex_Matrix ret;
		ret.real = this->real * m;
		ret.imag = this->imag * m;
		return ret;
	}

	Complex_Matrix operator* (GTYPE  value) {

		Complex_Matrix ret;
		ret.real = this->real * value;
		ret.imag = this->imag * value;
		return ret;
	}

	void show(const char* title) {
		printf("%s (%d, %d)\n", title, real.rows, real.cols);
		for (int i = 0; i < real.rows; i++) {
			for (int j = 0; j < real.cols; j++)
				printf("%9.5f + i*%9.5f,", real.data[i*real.cols + j], imag.data[i*real.cols + j]);
			printf("\n");
		}
		return;
	}

	Matrix real;
	Matrix imag;
};