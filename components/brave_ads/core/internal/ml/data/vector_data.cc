/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <utility>

#include "base/check_op.h"
#include "base/ranges/algorithm.h"

namespace brave_ads::ml {

namespace {
constexpr double kMinimumVectorLength = 1e-7;
}  // namespace

// An actual storage. Wrapped to a struct to make simple copy/move code.
// Two vectors are used to save the memory (see above how), because some models
// can consume a lot.
// There is two types of DataVectors:
// 1. The "dense" case: ({0, v0}, {1, v1}, .., {n, vn}}.
// We don't store first elements in this case, `points` is empty().
// 2. The sparse(general) case: ({p0, v0}, ..., {pn, vn}). We store points as
// {p0, .., pn} and values as {v0, .., vn}, points.size() == values.size().
class VectorDataStorage {
 public:
  VectorDataStorage() = default;
  VectorDataStorage(const size_t dimension_count,
                    std::vector<uint32_t> points,
                    std::vector<float> values)
      : dimension_count_(dimension_count),
        points_(std::move(points)),
        values_(std::move(values)) {
    CHECK(points_.size() == values_.size() || points_.empty());
  }

  size_t GetSize() const { return values_.size(); }

  uint32_t GetPointAt(size_t index) const {
    CHECK_LT(index, values_.size());
    if (points_.empty()) {  // The "dense" case, see the description.
      return index;
    }
    return points_[index];
  }

  std::vector<uint32_t>& points() { return points_; }
  const std::vector<uint32_t>& points() const { return points_; }
  std::vector<float>& values() { return values_; }
  const std::vector<float>& values() const { return values_; }
  size_t DimensionCount() const { return dimension_count_; }

 private:
  size_t dimension_count_ = 0;
  std::vector<uint32_t> points_;
  std::vector<float> values_;
};

VectorData::VectorData()
    : Data(DataType::kVector),
      storage_(std::make_unique<VectorDataStorage>()) {}

VectorData::VectorData(const VectorData& vector_data)
    : Data(DataType::kVector) {
  storage_ = std::make_unique<VectorDataStorage>(*vector_data.storage_);
}

VectorData::VectorData(VectorData&& vector_data) noexcept
    : Data(DataType::kVector) {
  storage_ = std::move(vector_data.storage_);
}

VectorData::VectorData(std::vector<float> data) : Data(DataType::kVector) {
  data.shrink_to_fit();
  storage_ = std::make_unique<VectorDataStorage>(
      data.size(), std::vector<uint32_t>(), std::move(data));
}

VectorData::VectorData(const size_t dimension_count,
                       const std::map<uint32_t, double>& data)
    : Data(DataType::kVector) {
  std::vector<uint32_t> points(data.size());
  std::vector<float> values(data.size());
  size_t i = 0;
  for (auto iter = data.cbegin(); iter != data.cend(); iter++, i++) {
    points[i] = iter->first;
    values[i] = static_cast<float>(iter->second);
  }
  storage_ = std::make_unique<VectorDataStorage>(
      dimension_count, std::move(points), std::move(values));
}

VectorData::~VectorData() = default;

VectorData& VectorData::operator=(const VectorData& vector_data) {
  storage_ = std::make_unique<VectorDataStorage>(*vector_data.storage_);
  return *this;
}

VectorData& VectorData::operator=(VectorData&& vector_data) noexcept {
  storage_ = std::move(vector_data.storage_);
  return *this;
}

float operator*(const VectorData& lhs, const VectorData& rhs) {
  if (lhs.IsEmpty() || rhs.IsEmpty()) {
    return std::numeric_limits<float>::quiet_NaN();
  }

  if (lhs.GetDimensionCount() != rhs.GetDimensionCount()) {
    return std::numeric_limits<float>::quiet_NaN();
  }

  float dot_product = 0.0;
  size_t lhs_index = 0;
  size_t rhs_index = 0;
  while (lhs_index < lhs.storage_->GetSize() &&
         rhs_index < rhs.storage_->GetSize()) {
    if (lhs.storage_->GetPointAt(lhs_index) ==
        rhs.storage_->GetPointAt(rhs_index)) {
      dot_product +=
          lhs.storage_->values()[lhs_index] * rhs.storage_->values()[rhs_index];
      ++lhs_index;
      ++rhs_index;
    } else {
      if (lhs.storage_->GetPointAt(lhs_index) <
          rhs.storage_->GetPointAt(rhs_index)) {
        ++lhs_index;
      } else {
        ++rhs_index;
      }
    }
  }

  return dot_product;
}

void VectorData::AddElementWise(const VectorData& other) {
  if (IsEmpty() || other.IsEmpty()) {
    return;
  }

  if (GetDimensionCount() != other.GetDimensionCount()) {
    return;
  }

  size_t index = 0;
  size_t other_index = 0;
  while (index < storage_->GetSize() &&
         other_index < other.storage_->GetSize()) {
    if (storage_->GetPointAt(index) ==
        other.storage_->GetPointAt(other_index)) {
      storage_->values()[index] += other.storage_->values()[other_index];
      ++index;
      ++other_index;
    } else {
      if (storage_->GetPointAt(index) <
          other.storage_->GetPointAt(other_index)) {
        ++index;
      } else {
        ++other_index;
      }
    }
  }
}

void VectorData::DivideByScalar(const float scalar) {
  if (IsEmpty()) {
    return;
  }

  for (float& value : storage_->values()) {
    value /= scalar;
  }
}

float VectorData::GetSum() const {
  return static_cast<float>(std::accumulate(
      storage_->values().cbegin(), storage_->values().cend(), 0.0,
      [](const float lhs, const float rhs) -> float { return lhs + rhs; }));
}

float VectorData::GetNorm() const {
  return static_cast<float>(sqrt(
      std::accumulate(storage_->values().cbegin(), storage_->values().cend(),
                      0.0, [](const float lhs, const float rhs) -> float {
                        return lhs + rhs * rhs;
                      })));
}

void VectorData::ToDistribution() {
  const float vector_sum = GetSum();
  if (vector_sum > kMinimumVectorLength) {
    for (float& value : storage_->values()) {
      value /= vector_sum;
    }
  }
}

void VectorData::Softmax() {
  float maximum = -std::numeric_limits<float>::infinity();
  for (float& value : storage_->values()) {
    maximum = (value > maximum) ? value : maximum;
  }
  float sum_exp = 0.0;
  for (float& value : storage_->values()) {
    sum_exp += std::exp(value - maximum);
  }
  for (float& value : storage_->values()) {
    value = std::exp(value - maximum) / sum_exp;
  }
}

void VectorData::Normalize() {
  const float vector_norm = GetNorm();
  if (vector_norm > kMinimumVectorLength) {
    for (float& value : storage_->values()) {
      value /= vector_norm;
    }
  }
}

void VectorData::Tanh() {
  for (float& value : storage_->values()) {
    value = tanh(value);
  }
}

float VectorData::ComputeSimilarity(const VectorData& other) const {
  CHECK_EQ(GetDimensionCount(), other.GetDimensionCount());

  return (*this * other) / (GetNorm() * other.GetNorm());
}

bool VectorData::IsEmpty() const {
  return GetDimensionCount() == 0;
}

size_t VectorData::GetDimensionCount() const {
  return storage_->DimensionCount();
}

size_t VectorData::GetNonZeroElementCount() const {
  if (IsEmpty()) {
    return 0;
  }

  return base::ranges::count_if(storage_->values(),
                                [](const float value) { return value != 0; });
}

const std::vector<float>& VectorData::GetData() const {
  return storage_->values();
}

std::vector<float> VectorData::GetDenseData() const {
  const size_t dimension_count = GetDimensionCount();
  if (storage_->values().size() == dimension_count) {
    return storage_->values();
  }

  std::vector<float> dense_vector(dimension_count);
  for (size_t i = 0; i < storage_->points().size(); i++) {
    dense_vector[storage_->points()[i]] = storage_->values()[i];
  }
  return dense_vector;
}

}  // namespace brave_ads::ml
