#include "matrix.h"
#include <iostream>
#include <iomanip>

Matrix::Matrix(int r, int c) {
	rows = r;
	cols = c;

	data.resize(rows, std::vector<double>(cols, 0.0));
}

void Matrix::set_value(int row, int col, double value) {
	data[row][col] = value;
}

void Matrix::display() const {
	for (int i = 0; i < rows; i++) {
		std::cout << "[ ";

		for (int j = 0; j < cols; j++) {
			std::cout << std::setw(6) << data[i][j];
		}

		std::cout << " ]" << std::endl;
	}
}

Matrix Matrix::add(const Matrix& other) const {
	
	//Check matrices have same dimension
	if (rows != other.rows || cols != other.cols) {
		std::cout << "Error: Matrices must have the same dimensions for addition." << std::endl;
		return Matrix(0, 0);
	}
	
	//Start adding
	Matrix result(rows, cols);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			result.data[i][j] = data[i][j] + other.data[i][j];
		}
	}

	return result;
}

Matrix Matrix::subtract(const Matrix& other) const {
	
	//Check matrices have same dimension
	if (rows != other.rows || cols != other.cols) {
		std::cout << "Error: Matrices must have the same dimensions for subtraction." << std::endl;
		return Matrix(0, 0);
	}

	Matrix result(rows, cols);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			result.data[i][j] = data[i][j] - other.data[i][j];
		}
	}

	return result;

}

Matrix Matrix::multiply(const Matrix& other) const {
	
	//Check the number of columns in the first matrix = the number of rows in the second matrix
	if (cols != other.rows) {
		std::cout << "Error: Number of columns in the first matrix must equal number of rows in the second matrix." << std::endl;
		return Matrix(0, 0);
	}

	Matrix result(rows, other.cols);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < other.cols; j++) {
			for (int k = 0; k < cols; k++) {
				result.data[i][j] += data[i][k] * other.data[k][j];
			}
		}
	}

	return result;

}

Matrix Matrix::transpose() const {
	Matrix result(cols, rows);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			result.data[j][i] = data[i][j];
		}
	}

	return result;

}

double Matrix::get_value(int row, int col) const {
	return data[row][col];
}

double Matrix::determinant() const {

	//Check the matrix is a squared matrix
	if (rows != cols) {
		std::cout << "Error: Determinant is only defined for square matrices." << std::endl;
		return 0.0;
	}

	//Base case: 1 x 1 matrix
	if (rows == 1) {
		return data[0][0];
	}

	//Base case: 2 x 2 matrix
	if (rows == 2) {
		return data[0][0] * data[1][1]
			- data[0][1] * data[1][0];
	}

	double det = 0.0;

	//Expand along the first row
	for (int j = 0; j < cols; j++) {

		Matrix minor(rows - 1, cols - 1);

		int minor_row = 0;

		for (int i = 1; i < rows; i++) {

			int minor_col = 0;

			for (int k = 0; k < cols; k++) {

				//Skip column j
				if (k == j) {
					continue;
				}

				minor.data[minor_row][minor_col] = data[i][k];
				minor_col++;
			}

			minor_row++;
		}

		double sign;

		if (j % 2 == 0) {
			sign = 1.0;
		}
		else {
			sign = -1.0;
		}

		det += sign * data[0][j] * minor.determinant();
	}

	return det;
}

Matrix Matrix::inverse() const {

	//Inverse only exists for square matrices
	if (rows != cols) {
		std::cout << "Error: Inverse is only defined for square matrices." << std::endl;
		return Matrix(0, 0);
	}

	//Compute determinant
	double det = determinant();

	//Check if matrix is singular
	if (det == 0) {
		std::cout << "Error: Matrix is singular and cannot be inverted." << std::endl;
		return Matrix(0, 0);
	}

	//1 x 1 matrix
	if (rows == 1) {
		Matrix result(1, 1);
		result.data[0][0] = 1.0 / data[0][0];
		return result;
	}

	//Build cofactor matrix
	Matrix cofactor(rows, cols);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {

			Matrix minor(rows - 1, cols - 1);

			int minor_row = 0;

			for (int r = 0; r < rows; r++) {

				if (r == i) {
					continue;
				}

				int minor_col = 0;

				for (int c = 0; c < cols; c++) {

					if (c == j) {
						continue;
					}

					minor.data[minor_row][minor_col] = data[r][c];
					minor_col++;
				}

				minor_row++;
			}

			double sign;

			if ((i + j) % 2 == 0) {
				sign = 1.0;
			}
			else {
				sign = -1.0;
			}

			cofactor.data[i][j] = sign * minor.determinant();
		}
	}

	//Transpose cofactor matrix to get adjugate
	Matrix adjugate = cofactor.transpose();

	//Divide adjugate by determinant
	Matrix result(rows, cols);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			result.data[i][j] = adjugate.data[i][j] / det;
		}
	}

	return result;
}
