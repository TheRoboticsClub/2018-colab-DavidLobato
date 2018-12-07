#ifndef JSON_NDARRAY_H
#define JSON_NDARRAY_H

#include <vector>
#include <json/json.h>

bool isNDArray(Json::Value const& value);
int asNDArrayRows(Json::Value const& value);
int asNDArrayCols(Json::Value const& value);

template <typename tScalar>
std::vector<tScalar> asNDArrayData(Json::Value const& value);

#endif /* JSON_NDARRAY_H */