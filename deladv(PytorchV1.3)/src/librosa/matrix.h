#pragma once
#include<vector>

#define GTYPE  float

class Matrix{
public:
	Matrix(){}
	Matrix(int32_t row,int32_t col){
		rows = row;
		cols = col;
		data.resize(row*col);
	}

	Matrix(int32_t row, int32_t col, GTYPE value) {
		rows = row;
		cols = col;
		data.resize(row * col);
#pragma omp parallel for
		for (int i = 0; i < data.size(); i++)
			data[i] = value;
	}


	Matrix(const Matrix& cls) {
		rows = cls.rows;
		cols = cls.cols;
		data.resize(rows * cols);
#pragma omp parallel for
		for (int i = 0; i < data.size(); i++)
			data[i] = cls.data[i];
	}	

	Matrix& operator=(const Matrix& cls) {

		if (this != &cls) {
			rows = cls.rows;
			cols = cls.cols;
			data.resize(rows * cols);
#pragma omp parallel for
			for (int i = 0; i < data.size(); i++)
				data[i] = cls.data[i];
		}		
		return *this;
	}

	inline bool empty() { return rows == 0;}

	inline void reset_type(const Matrix& cls) {
		rows = cls.rows;
		cols = cls.cols;
		data.resize(rows * cols);
	}

	inline void reset_type(int32_t row, int32_t col) {
		rows = row;
		cols = col;
		data.resize(rows * cols);
	}

	inline int32_t get_row(int32_t index){
		return index / cols;
	}

	inline int32_t get_col(int32_t index) {
		return index % cols;
	}

	inline int32_t get_index(int32_t r, int32_t c) {
		return r * cols + c;
	}

	inline void transpos() {

		std::vector<GTYPE> temp = data;
		int32_t c = cols;
		cols = rows;
		rows = c;
#pragma omp parallel for
		for (int i = 0; i < data.size(); i++) {
			data[i] = temp[get_col(i) * rows + get_row(i)];
		}
	}	

	~Matrix(){}


	std::vector<GTYPE> data;
	int32_t rows = 0;
	int32_t cols = 0;
private:

};