// based on gemlowp doc/quantization_example.cc

#include <gemmlowp.h>
#include <output_stages.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>
#include "json_ndarray.h"

const auto kOrder = gemmlowp::MapOrder::RowMajor;

// We will handle both float and quantized matrices, which we will
// represent as gemmlowp::MatrixMap.
// We will need to be able to print them.

// Output a matrix to a std::ostream
template <typename tScalar, gemmlowp::MapOrder tOrder>
std::ostream& operator<<(std::ostream& s,
                         const gemmlowp::MatrixMap<tScalar, tOrder>& m) {
  for (int i = 0; i < m.rows(); i++) {
    for (int j = 0; j < m.cols(); j++) {
      if (j) {
        s << '\t';
      }
      s << static_cast<float>(m(i, j));
    }
    s << '\n';
  }
  return s;
}

typedef float (*activation_f_t)(float);

float Relu(float value) { return std::max(value, 0.f); }

float Sigmoid(float value) { return 1.f / (1.f + std::exp(-value)); }

void FloatFullyConnected(
    const gemmlowp::MatrixMap<const float, kOrder>& input,
    const gemmlowp::MatrixMap<const float, kOrder>& weights,
    const gemmlowp::MatrixMap<const float, kOrder>& bias,
    activation_f_t activation_f,
    gemmlowp::MatrixMap<float, kOrder>* result) {
  assert(input.cols() == weights.rows());
  assert(input.rows() == result->rows());
  assert(weights.cols() == bias.cols());
  assert(weights.cols() == result->cols());
  for (int i = 0; i < input.rows(); i++) {
    for (int k = 0; k < weights.cols(); k++) {
      float accum = 0;

      for (int j = 0; j < input.cols(); j++) {
        accum += input(i, j) * weights(j, k);
      }

      accum += bias(0, k);

      (*result)(i, k) = activation_f ? activation_f(accum) : accum;
    }
  }
}

template <typename tScalar, gemmlowp::MapOrder tOrder>
class MatrixWithStorage {
 public:
  MatrixWithStorage(int rows, int cols)
      : storage(rows * cols), matrix_map(storage.data(), rows, cols) {}
  
  gemmlowp::MatrixMap<const tScalar, tOrder> ConstMap() const {
    return gemmlowp::MatrixMap<const tScalar, tOrder>(
        storage.data(), matrix_map.rows(), matrix_map.cols());
  }
  gemmlowp::MatrixMap<tScalar, tOrder> Map() {
    return gemmlowp::MatrixMap<tScalar, tOrder>(
        storage.data(), matrix_map.rows(), matrix_map.cols());
  }
  const std::vector<tScalar>& Storage() const { return storage; }
  std::vector<tScalar>& Storage() { return storage; }

 private:
  std::vector<tScalar> storage;
  gemmlowp::MatrixMap<tScalar, tOrder> matrix_map;
};

template <typename tScalar, gemmlowp::MapOrder tOrder>
std::ostream& operator<<(std::ostream& s,
                         const MatrixWithStorage<tScalar, tOrder>& m) {
  return s << m.ConstMap();
}

int main(int argc, char* argv[]) {
  std::cout.precision(3);

  Json::Value root;

  std::cin >> root;

  // input
  static const float input_data_array[] = {0, 0, 0, 1, 1, 0, 1, 1};
  std::vector<float> input_data(
      input_data_array, input_data_array + sizeof(input_data_array) /
                                               sizeof(input_data_array[0]));
  const int input_rows = 4;
  const int input_cols = 2;
  MatrixWithStorage<float, kOrder> input(input_rows, input_cols);
  input.Storage() = input_data;

  // dense/bias
  const int dense_bias_rows = asNDArrayRows(root["dense/bias:0"]);
  const int dense_bias_cols = asNDArrayCols(root["dense/bias:0"]);
  auto dense_bias_data = asNDArrayData<float>(root["dense/bias:0"]);
  MatrixWithStorage<float, kOrder> dense_bias(dense_bias_rows, dense_bias_cols);
  dense_bias.Storage() = dense_bias_data;

  // dense/kernel
  const int dense_weights_rows = asNDArrayRows(root["dense/kernel:0"]);
  const int dense_weights_cols = asNDArrayCols(root["dense/kernel:0"]);
  auto dense_weights_data = asNDArrayData<float>(root["dense/kernel:0"]);
  MatrixWithStorage<float, kOrder> dense_kernel(dense_weights_rows,
                                                dense_weights_cols);
  dense_kernel.Storage() = dense_weights_data;

  // dense_1/bias
  const int dense_1_bias_rows = asNDArrayRows(root["dense_1/bias:0"]);
  const int dense_1_bias_cols = asNDArrayCols(root["dense_1/bias:0"]);
  auto dense_1_bias_data = asNDArrayData<float>(root["dense_1/bias:0"]);
  MatrixWithStorage<float, kOrder> dense_1_bias(dense_1_bias_rows,
                                                dense_1_bias_cols);
  dense_1_bias.Storage() = dense_1_bias_data;

  // dense_1/kernel
  const int dense_1_weights_rows = asNDArrayRows(root["dense_1/kernel:0"]);
  const int dense_1_weights_cols = asNDArrayCols(root["dense_1/kernel:0"]);
  auto dense_1_weights_data = asNDArrayData<float>(root["dense_1/kernel:0"]);
  MatrixWithStorage<float, kOrder> dense_1_kernel(dense_1_weights_rows,
                                                  dense_1_weights_cols);
  dense_1_kernel.Storage() = dense_1_weights_data;

  // layer 0 activation
  MatrixWithStorage<float, kOrder> activation_layer0(input_rows,
                                                     dense_weights_cols);
  auto activation_layer0_map = activation_layer0.Map();
  FloatFullyConnected(input.ConstMap(), dense_kernel.ConstMap(),
                      dense_bias.ConstMap(), Relu, &activation_layer0_map);

  // layer 1 activation (output)
  MatrixWithStorage<float, kOrder> activation_layer1(
      activation_layer0_map.rows(), dense_1_weights_cols);
  auto activation_layer1_map = activation_layer1.Map();
  FloatFullyConnected(activation_layer0.ConstMap(), dense_1_kernel.ConstMap(),
                      dense_1_bias.ConstMap(), 0 /*activation=Identity*/, &activation_layer1_map);

  std::cout << "Here is input matrix:\n" << input << std::endl;
  std::cout << "Here is dense/bias matrix:\n" << dense_bias << std::endl;
  std::cout << "Here is dense/kernel matrix:\n" << dense_kernel << std::endl;
  std::cout << "Here is dense_1/bias matrix:\n" << dense_1_bias << std::endl;
  std::cout << "Here is dense_1/kernel matrix:\n"
            << dense_1_kernel << std::endl;
  std::cout << "Here is activation (Relu) layer 0 matrix:\n"
            << activation_layer0 << std::endl;
  std::cout << "Here is activation (Identity) layer 1 matrix:\n"
            << activation_layer1 << std::endl;
}