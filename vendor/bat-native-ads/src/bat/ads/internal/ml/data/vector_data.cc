/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/data/vector_data.h"

#include <limits>
#include <numeric>
#include <utility>

namespace ads {
namespace ml {

namespace {
constexpr double kMinimumVectorLength = 1e-7;
}  // namespace

VectorData::VectorData() : Data(DataType::kVector) {}

VectorData::VectorData(const VectorData& vector_data)
    : Data(DataType::kVector) {
  dimension_count_ = vector_data.dimension_count_;
  data_ = vector_data.data_;
}

VectorData::VectorData(VectorData&& vector_data) : Data(DataType::kVector) {
  dimension_count_ = vector_data.dimension_count_;
  data_ = std::move(vector_data.data_);
}

VectorData::VectorData(const std::vector<float>& data)
    : Data(DataType::kVector) {
  dimension_count_ = static_cast<int>(data.size());
  data_.resize(dimension_count_);
  for (int i = 0; i < dimension_count_; ++i) {
    data_[i] = SparseVectorElement(static_cast<uint32_t>(i), data[i]);
  }
}

VectorData::VectorData(const int dimension_count,
                       const std::map<uint32_t, double>& data)
    : Data(DataType::kVector) {
  dimension_count_ = dimension_count;
  data_.reserve(dimension_count_);
  for (auto iter = data.cbegin(); iter != data.cend(); iter++) {
    data_.push_back(SparseVectorElement(iter->first, iter->second));
  }
}

VectorData::~VectorData() = default;

VectorData& VectorData::operator=(const VectorData& vector_data) {
  dimension_count_ = vector_data.GetDimensionCount();
  data_ = vector_data.data_;
  return *this;
}

VectorData& VectorData::operator=(VectorData&& vector_data) {
  dimension_count_ = vector_data.dimension_count_;
  data_ = std::move(vector_data.data_);
  return *this;
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
      dot_product += static_cast<double>(lhs.data_[lhs_index].second) *
                     rhs.data_[rhs_index].second;
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

void VectorData::Normalize() {
  const auto vector_length = sqrt(std::accumulate(
      data_.cbegin(), data_.cend(), 0.0,
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

}  // namespace ml
}  // namespace ads
