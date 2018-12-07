// based on gemlowp doc/quantization_example.cc

#include <gemmlowp.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#include <limits>
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

// A structure to hold quantization parameters 'scale' and 'zero_point'
// as discussed in doc/quantization.md. As explained there, the meaning
// of these values is as the constants in the quantization equation
//
//   real_value = scale * (quantized_value - zero_point)
//
// In other words, 'zero_point' is the quantized value that corresponds
// to the real value 0, and 'scale' is the difference of real values
// corresponding to consecutive quantized values.
struct QuantizationParams {
  float scale;
  std::uint8_t zero_point;
};

// Given the min and max values of a float array, return
// reasonable quantization parameters to use for this array.
QuantizationParams ChooseQuantizationParams(float min, float max) {
  // We extend the [min, max] interval to ensure that it contains 0.
  // Otherwise, we would not meet the requirement that 0 be an exactly
  // representable value.
  min = std::min(min, 0.f);
  max = std::max(max, 0.f);

  // the min and max quantized values, as floating-point values
  const float qmin = 0;
  const float qmax = 255;

  // First determine the scale.
  const double scale = (max - min) / (qmax - qmin);

  // Zero-point computation.
  // First the initial floating-point computation. The zero-point can be
  // determined from solving an affine equation for any known pair
  // (real value, corresponding quantized value).
  // We know two such pairs: (rmin, qmin) and (rmax, qmax).
  // Let's use the first one here.
  const double initial_zero_point = qmin - min / scale;

  // Now we need to nudge the zero point to be an integer
  // (our zero points are integer, and this is motivated by the requirement
  // to be able to represent the real value "0" exactly as a quantized value,
  // which is required in multiple places, for example in Im2col with SAME
  // padding).
  std::uint8_t nudged_zero_point = 0;
  if (initial_zero_point < qmin) {
    nudged_zero_point = qmin;
  } else if (initial_zero_point > qmax) {
    nudged_zero_point = qmax;
  } else {
    nudged_zero_point =
        static_cast<std::uint8_t>(std::round(initial_zero_point));
  }

  QuantizationParams result;
  result.scale = scale;
  result.zero_point = nudged_zero_point;
  return result;
}

template <typename tScalar>
void Quantize(const QuantizationParams& qparams, const std::vector<float>& src,
              std::vector<tScalar>* dst) {
  assert(src.size() == dst->size());
  for (std::size_t i = 0; i < src.size(); i++) {
    const float real_val = src[i];
    const float transformed_val = qparams.zero_point + real_val / qparams.scale;
    const float clamped_val = std::max(
      static_cast<float>(std::numeric_limits<tScalar>::min()),
        std::min(static_cast<float>(std::numeric_limits<tScalar>::max()),
                      transformed_val));
    (*dst)[i] = static_cast<tScalar>(std::round(clamped_val));
  }
}

template <typename tScalar>
void Dequantize(const QuantizationParams& qparams,
                const std::vector<tScalar>& src, std::vector<float>* dst) {
  assert(src.size() == dst->size());
  for (std::size_t i = 0; i < src.size(); i++) {
    const tScalar quantized_val = src[i];
    (*dst)[i] = qparams.scale * (quantized_val - qparams.zero_point);
  }
}

// Given a real_multiplier in the interval (0, 1),
// produces a pair (quantized_multiplier, right_shift) where
// quantized_multiplier is an int32 representing a fixed-point value
// in the interval [-1, 1)  (in practice we only produce positive values)
// and right_shift is an amount to shift right by, so that the
// floating-point multiplication of some int32 input value by real_multiplier,
//
//   return static_cast<int32>(int32_value * real_multiplier);
//
// is best approximated by the integer-arithmetic-only code
//
//   return RoundingRightShift(
//       FixedPointMultiplication(int32_value, quantized_multiplier),
//       right_shift);
//
// This is how to obtain the fixed-point multiplier and right shift
// parameters to pass to
// OutputStageQuantizeDownInt32ByFixedPoint.
//
// Note: all this code only needs to run offline to generate the quantized
// neural network workload, not at runtime on the
// device on which quantized neural networks need to run. So it's not
// performance-critical at all.
void QuantizeMultiplierSmallerThanOne(float real_multiplier,
                                      std::int32_t* quantized_multiplier,
                                      int* right_shift) {
  assert(real_multiplier > 0.f);
  assert(real_multiplier < 1.f);
  int s = 0;
  // We want to bring the real multiplier into the interval [1/2, 1).
  // We can do so by multiplying it by two, and recording how many times
  // we multiplied by two so that we can compensate that by a right
  // shift by the same amount.
  while (real_multiplier < 0.5f) {
    real_multiplier *= 2.0f;
    s++;
  }
  // Now that the real multiplier is in [1/2, 1), we convert it
  // into a fixed-point number.
  std::int64_t q =
      static_cast<std::int64_t>(std::round(real_multiplier * (1ll << 31)));
  assert(q <= (1ll << 31));
  // Handle the special case when the real multiplier was so close to 1
  // that its fixed-point approximation was undistinguishable from 1.
  // We handle this by dividing it by two, and remembering to decrement
  // the right shift amount.
  if (q == (1ll << 31)) {
    q /= 2;
    s--;
  }
  assert(s >= 0);
  assert(q <= std::numeric_limits<std::int32_t>::max());
  *quantized_multiplier = static_cast<std::int32_t>(q);
  *right_shift = s;
}

int32_t MultiplyByQuantizedMultiplierSmallerThanOne(
    int32_t x, int32_t quantized_multiplier, int right_shift) {
  using gemmlowp::RoundingDivideByPOT;
  using gemmlowp::SaturatingRoundingDoublingHighMul;
  return RoundingDivideByPOT(
      SaturatingRoundingDoublingHighMul(x, quantized_multiplier), right_shift);
}

void QuantizedFullyConnected(
    const gemmlowp::MatrixMap<const uint8_t, kOrder>& input, int32_t input_zero_point,
    const gemmlowp::MatrixMap<const uint8_t, kOrder>& weights, int32_t weights_zero_point,
    const gemmlowp::MatrixMap<const int32_t, kOrder>& bias,
    int32_t output_min,
    int32_t output_max,
    int32_t output_multiplier,
    int output_shift,
    gemmlowp::MatrixMap<uint8_t, kOrder>* result, int32_t result_zero_point) {
  assert(input.cols() == weights.rows());
  assert(input.rows() == result->rows());
  assert(weights.cols() == bias.cols());
  assert(weights.cols() == result->cols());
  for (int i = 0; i < input.rows(); i++) {
    for (int k = 0; k < weights.cols(); k++) {
      int32_t accum = 0;

      for (int j = 0; j < input.cols(); j++) {
        accum += (input(i, j) - input_zero_point) * (weights(j, k) - weights_zero_point);
      }

      accum += bias(0, k);

      accum = MultiplyByQuantizedMultiplierSmallerThanOne(accum, output_multiplier, output_shift);
      accum += result_zero_point;

      accum = std::max(accum, output_min);
      accum = std::min(accum, output_max);

      (*result)(i, k) = static_cast<uint8_t>(accum);
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
  MatrixWithStorage<std::uint8_t, kOrder> input_uint8(input_rows, input_cols);
  input.Storage() = input_data;

  // dense/bias
  const int dense_bias_rows = asNDArrayRows(root["dense/bias:0"]);
  const int dense_bias_cols = asNDArrayCols(root["dense/bias:0"]);
  auto dense_bias_data = asNDArrayData<float>(root["dense/bias:0"]);
  MatrixWithStorage<float, kOrder> dense_bias(dense_bias_rows, dense_bias_cols);
  MatrixWithStorage<std::int32_t, kOrder> dense_bias_int32(dense_bias_rows, dense_bias_cols);
  dense_bias.Storage() = dense_bias_data;

  // dense/weights
  const int dense_weights_rows = asNDArrayRows(root["dense/kernel:0"]);
  const int dense_weights_cols = asNDArrayCols(root["dense/kernel:0"]);
  auto dense_weights_data = asNDArrayData<float>(root["dense/kernel:0"]);
  MatrixWithStorage<float, kOrder> dense_weights(dense_weights_rows, dense_weights_cols);
  MatrixWithStorage<std::uint8_t, kOrder> dense_weights_uint8(dense_weights_rows, dense_weights_cols);
  dense_weights.Storage() = dense_weights_data;

  //dense/weights_quant/min,max
  const float dense_weights_quant_min = root["dense/weights_quant/min:0"].asFloat();
  const float dense_weights_quant_max = root["dense/weights_quant/max:0"].asFloat();

  //dense/act_quant/min,max
  const float dense_act_quant_min = root["dense/act_quant/min:0"].asFloat();
  const float dense_act_quant_max = root["dense/act_quant/max:0"].asFloat();


  // dense_1/bias
  const int dense_1_bias_rows = asNDArrayRows(root["dense_1/bias:0"]);
  const int dense_1_bias_cols = asNDArrayCols(root["dense_1/bias:0"]);
  auto dense_1_bias_data = asNDArrayData<float>(root["dense_1/bias:0"]);
  MatrixWithStorage<float, kOrder> dense_1_bias(dense_1_bias_rows, dense_1_bias_cols);
  MatrixWithStorage<std::int32_t, kOrder> dense_1_bias_int32(dense_1_bias_rows, dense_1_bias_cols);
  dense_1_bias.Storage() = dense_1_bias_data;

  // dense_1/weights
  const int dense_1_weights_rows = asNDArrayRows(root["dense_1/kernel:0"]);
  const int dense_1_weights_cols = asNDArrayCols(root["dense_1/kernel:0"]);
  auto dense_1_weights_data = asNDArrayData<float>(root["dense_1/kernel:0"]);
  MatrixWithStorage<float, kOrder> dense_1_weights(dense_1_weights_rows, dense_1_weights_cols);
  MatrixWithStorage<std::uint8_t, kOrder> dense_1_weights_uint8(dense_1_weights_rows, dense_1_weights_cols);
  dense_1_weights.Storage() = dense_1_weights_data;

  //dense_1/weights_quant/min,max
  const float dense_1_weights_quant_min = root["dense_1/weights_quant/min:0"].asFloat();
  const float dense_1_weights_quant_max = root["dense_1/weights_quant/max:0"].asFloat();

  //dense_1/act_quant/min,max
  const float dense_1_act_quant_min = root["dense_1/act_quant/min:0"].asFloat();
  const float dense_1_act_quant_max = root["dense_1/act_quant/max:0"].asFloat();

  std::cout << "Here is input matrix:\n" << input << std::endl;
  std::cout << "Here is dense/bias matrix:\n" << dense_bias << std::endl;
  std::cout << "Here is dense/weights matrix:\n" << dense_weights << std::endl;
  std::cout << "Here is dense/weights_quant: min=" << dense_weights_quant_min
            << ", max=" << dense_weights_quant_max << std::endl;
  std::cout << "Here is dense/act_quant: min=" << dense_act_quant_min
            << ", max=" << dense_act_quant_max << std::endl;

  std::cout << "Here is dense_1/bias matrix:\n" << dense_1_bias << std::endl;
  std::cout << "Here is dense_1/weights matrix:\n"
            << dense_1_weights << std::endl;
  std::cout << "Here is dense_1/weights_quant: min=" << dense_1_weights_quant_min
            << ", max=" << dense_1_weights_quant_max << std::endl;
  std::cout << "Here is dense_1/act_quant: min=" << dense_1_act_quant_min
            << ", max=" << dense_1_act_quant_max << std::endl;

  
  // calculate quantization params
  const auto input_qparams = ChooseQuantizationParams(0.f, 1.f);
  const auto dense_weights_qparams = ChooseQuantizationParams(dense_weights_quant_min, dense_weights_quant_max);
  const QuantizationParams dense_bias_qparams = {.scale = input_qparams.scale * dense_weights_qparams.scale, .zero_point = 0};
  const auto dense_act_qparams = ChooseQuantizationParams(dense_act_quant_min, dense_act_quant_max);
  const auto dense_1_weights_qparams = ChooseQuantizationParams(dense_1_weights_quant_min, dense_1_weights_quant_max);
  const QuantizationParams dense_1_bias_qparams = {.scale = dense_act_qparams.scale * dense_1_weights_qparams.scale, .zero_point = 0};
  const auto dense_1_act_qparams = ChooseQuantizationParams(dense_1_act_quant_min, dense_1_act_quant_max);

  Quantize(input_qparams, input.Storage(), &input_uint8.Storage());
  Quantize(dense_weights_qparams, dense_weights.Storage(), &dense_weights_uint8.Storage());
  Quantize(dense_bias_qparams, dense_bias.Storage(), &dense_bias_int32.Storage());
  Quantize(dense_1_weights_qparams, dense_1_weights.Storage(), &dense_1_weights_uint8.Storage());
  Quantize(dense_1_bias_qparams, dense_1_bias.Storage(), &dense_1_bias_int32.Storage());

  std::cout << "Here is quantized input matrix:\n" << input_uint8 << std::endl;
  std::cout << "Here is quantized dense/bias matrix:\n" << dense_bias_int32 << std::endl;
  std::cout << "Here is quantized dense/weights matrix:\n" << dense_weights_uint8 << std::endl;
  std::cout << "Here is quantized dense_1/bias matrix:\n" << dense_1_bias_int32 << std::endl;
  std::cout << "Here is quantized dense_1/weights matrix:\n" << dense_1_weights_uint8 << std::endl;

  std::int32_t dense_quantized_multiplier;
  int dense_right_shift;
  QuantizeMultiplierSmallerThanOne(
      (input_qparams.scale * dense_weights_qparams.scale /
       dense_act_qparams.scale),
      &dense_quantized_multiplier, &dense_right_shift);

  std::int32_t dense_1_quantized_multiplier;
  int dense_1_right_shift;
  QuantizeMultiplierSmallerThanOne(
      (dense_act_qparams.scale * dense_1_weights_qparams.scale /
       dense_1_act_qparams.scale),
      &dense_1_quantized_multiplier, &dense_1_right_shift);

  std::cout << "End of OFFLINE QUANTIZATION CODE.\n" << std::endl;

  // TODO: is efficient zero_point handling needed on HW??
  // // do packing: 'Packed' means that it is laid out in the storage order
  // gemmlowp::SideMap<uint8_t, gemmlowp::SideMapOrder::WidthMajor>
  //     input_uint8_packed(input_uint8.Map().data(), input_uint8.Map().rows(),
  //                        input_uint8.Map().cols(), input_uint8.Map().stride());
  // gemmlowp::SideMap<uint8_t, gemmlowp::SideMapOrder::DepthMajor>
  //     dense_weights_uint8_packed(
  //         dense_weights_uint8.Map().data(), dense_weights_uint8.Map().cols(),
  //         dense_weights_uint8.Map().rows(), dense_weights_uint8.Map().stride());
  // // calculate sum of rows on input_uint8
  // std::vector<int32_t> sums_of_each_slice_input(input_rows);
  // for (int w = 0; w < input_uint8_packed.width(); ++w) {
  //   for (int d = 0; d < input_uint8_packed.depth(); ++d) {
  //     sums_of_each_slice_input[w] += input_uint8_packed(w, d);
  //   }
  //   std::cout << "sums_of_each_slice_input[" << w
  //             << "]=" << sums_of_each_slice_input[w] << std::endl;
  // }

  // // calculate sum of cols on rhs
  // std::vector<int32_t> sums_of_each_slice_dense_weights(dense_weights_cols);
  // for (int w = 0; w < dense_weights_uint8_packed.width(); ++w) {
  //   for (int d = 0; d < dense_weights_uint8_packed.depth(); ++d) {
  //     sums_of_each_slice_dense_weights[w] += dense_weights_uint8_packed(w, d);
  //   }
  //   std::cout << "sums_of_each_slice_dense_weights[" << w
  //             << "]=" << sums_of_each_slice_dense_weights[w] << std::endl;
  // }

  // //layer 0 activation
  MatrixWithStorage<float, kOrder> activation_layer0(input_rows,
                                                     dense_weights_cols);
  MatrixWithStorage<uint8_t, kOrder> activation_layer0_uint8(input_rows, dense_weights_cols);
  auto activation_layer0_uint8_map = activation_layer0_uint8.Map();
  QuantizedFullyConnected(
      input_uint8.ConstMap(), input_qparams.zero_point,
      dense_weights_uint8.ConstMap(), dense_weights_qparams.zero_point,
      dense_bias_int32.ConstMap(), 0, INT32_MAX, dense_quantized_multiplier,
      dense_right_shift, &activation_layer0_uint8_map, dense_act_qparams.zero_point);

  // //layer 1 activation (output)
  MatrixWithStorage<float, kOrder> activation_layer1(
      activation_layer0.ConstMap().rows(), dense_1_weights_cols);
  MatrixWithStorage<uint8_t, kOrder> activation_layer1_uint8(activation_layer0_uint8_map.rows(), dense_1_weights_cols);
  auto activation_layer1_uint8_map = activation_layer1_uint8.Map();
  QuantizedFullyConnected(
      activation_layer0_uint8.ConstMap(), dense_act_qparams.zero_point,
      dense_1_weights_uint8.ConstMap(), dense_1_weights_qparams.zero_point,
      dense_1_bias_int32.ConstMap(), 0, INT32_MAX, dense_1_quantized_multiplier,
      dense_1_right_shift, &activation_layer1_uint8_map, dense_1_act_qparams.zero_point);


  //dequantize layer outputs
  Dequantize(dense_act_qparams, activation_layer0_uint8.Storage(), &activation_layer0.Storage());
  Dequantize(dense_1_act_qparams, activation_layer1_uint8.Storage(), &activation_layer1.Storage());

  std::cout << "Here is quantized activation layer 0 matrix:\n" << activation_layer0_uint8 << std::endl;
  std::cout << "Here is quantized activation layer 1 matrix:\n" << activation_layer1_uint8 << std::endl;
  std::cout << "Here is dequantized activation layer 0 matrix:\n" << activation_layer0 << std::endl;
  std::cout << "Here is dequantized activation layer 1 matrix:\n" << activation_layer1 << std::endl;
}