/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include <map>
#include <numeric>
#include <vector>

#include "bat/ads/internal/ml/data/vector_data.h"

namespace ads {
namespace ml {

namespace {
const double kMinimumVectorLength = 1e-7;
}  // namespace

VectorData::VectorData() : Data(DataType::VECTOR_DATA) {}

VectorData::VectorData(const VectorData& vector_data)
    : Data(DataType::VECTOR_DATA) {
  dimension_count_ = vector_data.GetDimensionCount();
  data_ = vector_data.GetRawData();
}

VectorData& VectorData::operator=(const VectorData& vector_data) {
  dimension_count_ = vector_data.GetDimensionCount();
  data_ = vector_data.GetRawData();
  return *this;
}

VectorData::~VectorData() = default;

VectorData::VectorData(const int dimension_count,
                       const std::map<uint32_t, double>& data)
    : Data(DataType::VECTOR_DATA) {
  dimension_count_ = dimension_count;
  data_.reserve(dimension_count_);
  for (auto iter = data.begin(); iter != data.end(); iter++) {
    data_.push_back(SparseVectorElement(iter->first, iter->second));
  }
}

VectorData::VectorData(const std::vector<double>& data)
    : Data(DataType::VECTOR_DATA) {
  dimension_count_ = static_cast<int>(data.size());
  data_.resize(dimension_count_);
  for (int i = 0; i < dimension_count_; ++i) {
    data_[i] = SparseVectorElement(static_cast<uint32_t>(i), data[i]);
  }
}

void VectorData::Normalize() {
  const double vector_length = sqrt(std::accumulate(
      data_.begin(), data_.end(), 0.0,
      [](const double& lhs, const SparseVectorElement& rhs) -> double {
        return lhs + rhs.second * rhs.second;
      }));
  if (vector_length > kMinimumVectorLength) {
    for (size_t i = 0; i < data_.size(); ++i) {
      data_[i].second /= vector_length;
    }
  }
}

int VectorData::GetDimensionCount() const {
  return dimension_count_;
}

std::vector<SparseVectorElement> VectorData::GetRawData() const {
  return data_;
}

double operator*(const VectorData& lhs, const VectorData& rhs) {
  if (!lhs.dimension_count_ || !rhs.dimension_count_) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  if (lhs.dimension_count_ != rhs.dimension_count_) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  double dot_product = 0.0;
  size_t lhs_index = 0;
  size_t rhs_index = 0;
  while (lhs_index < lhs.data_.size() && rhs_index < rhs.data_.size()) {
    if (lhs.data_[lhs_index].first == rhs.data_[rhs_index].first) {
      dot_product += lhs.data_[lhs_index].second * rhs.data_[rhs_index].second;
      ++lhs_index;
      ++rhs_index;
    } else {
      if (lhs.data_[lhs_index].first < rhs.data_[rhs_index].first) {
        ++lhs_index;
      } else {
        ++rhs_index;
      }
    }
  }

  return dot_product;
}

}  // namespace ml
}  // namespace ads
