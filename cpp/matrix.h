
#ifndef MATRIX_H
#define MATRIX_H

#include <string>
#include <vector>

class Matrix {
private:
    int rows;
    int cols;
    std::vector<std::vector<double>>data;

public:
    Matrix(int r, int c);

    Matrix add(const Matrix& other) const;
    Matrix subtract(const Matrix& other) const;
    Matrix multiply(const Matrix& other) const;
    Matrix transpose() const;
    
    double get_value(int row, int col) const;

    void display() const;

    void set_value(int row, int col, double value);

    double determinant() const;

    Matrix inverse() const;

};

#endif
