#include "json_ndarray.h"

bool isNDArray(Json::Value const& value) {
  const Json::Value& paramTypeAttr = value["type"];
  const Json::Value& paramValueAttr = value["value"];

  return paramTypeAttr.isString() && (paramTypeAttr.asString() == "ndarray") &&
         paramValueAttr.isArray();
}

int asNDArrayRows(Json::Value const& value) {
  const Json::Value& paramValueAttr = value["value"];

  // empty ndarray
  if (paramValueAttr.empty()) return 0;

  // one row ndarray rows=1, else rows=value.size()
  return paramValueAttr[0].isArray() ? paramValueAttr.size() : 1;
}

int asNDArrayCols(Json::Value const& value) {
  const Json::Value& paramValueAttr = value["value"];

  // empty ndarray
  if (paramValueAttr.empty()) return 0;

  // one row ndarray cols=value.size(), else cols=value[0].size()
  return paramValueAttr[0].isArray() ? paramValueAttr[0].size()
                                     : paramValueAttr.size();
}

size_t asNDArraySize(Json::Value const& value) {
  return asNDArrayRows(value) * asNDArrayCols(value);
}

template <typename tScalar>
std::vector<tScalar> asNDArrayData(Json::Value const& value) {
  const int rows = asNDArrayRows(value);
  const int cols = asNDArrayCols(value);
  const size_t size = rows * cols;
  std::vector<tScalar> data(size);

  if (0 != size) {
    const Json::Value& paramValueAttr = value["value"];

    if (1 == rows) {  // one row ndarray
      for (Json::ArrayIndex i = 0; i < paramValueAttr.size(); i++) {
        data[i] = static_cast<tScalar>(paramValueAttr[i].asDouble());
      }
    } else {
      for (Json::ArrayIndex i = 0; i < paramValueAttr.size(); i++) {
        const Json::Value& row = paramValueAttr[i];
        for (Json::ArrayIndex j = 0; j < row.size(); j++) {
          data[(i * cols) + j] =
              static_cast<tScalar>(row[j].asDouble());
        }
      }
    }
  }

  return data;
}

template std::vector<float> asNDArrayData<float>(Json::Value const& value);
template std::vector<int> asNDArrayData<int>(Json::Value const& value);