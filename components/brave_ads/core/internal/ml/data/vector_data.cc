/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

#include <cstddef>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

namespace brave_ads::ml {

namespace {
constexpr double kMinimumVectorLength = 1e-7;
}  // namespace

// An actual storage. Wrapped to a struct to make simple copy/move code.
// Two vectors are used to save the memory (see above how), because some models
// can consume a lot.
// There is two types of DataVectors:
// 1. The "dense" case: ({0, v0}, {1, v1}, .., {n, vn}}.
// We don't store first elements in this case, |points| is empty().
// 2. The sparse(general) case: ({p0, v0}, ..., {pn, vn}). We store points as
// {p0, .., pn} and values as {v0, .., vn}, points.size() == values.size().
class VectorDataStorage {
 public:
  VectorDataStorage() = default;
  VectorDataStorage(const int dimension_count,
                    std::vector<uint32_t> points,
                    std::vector<float> values)
      : dimension_count_(dimension_count),
        points_(std::move(points)),
        values_(std::move(values)) {
    DCHECK((points_.size() == values_.size() || points_.empty()));
  }

  size_t GetSize() const { return values_.size(); }

  uint32_t GetPointAt(size_t index) const {
    DCHECK_LT(index, values_.size());
    if (points_.empty()) {  // The "dense" case, see the description.
      return index;
    }
    return points_[index];
  }

  std::vector<float>& values() { return values_; }
  const std::vector<float>& values() const { return values_; }
  int DimensionCount() const { return dimension_count_; }

 private:
  int dimension_count_ = 0;
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
      static_cast<int>(data.size()), std::vector<uint32_t>(), std::move(data));
}

VectorData::VectorData(int dimension_count,
                       const std::map<uint32_t, double>& data)
    : Data(DataType::kVector) {
  std::vector<uint32_t> points(data.size());
  std::vector<float> values(data.size());
  size_t i = 0;
  for (auto iter = data.cbegin(); iter != data.cend(); iter++, i++) {
    points[i] = iter->first;
    values[i] = iter->second;
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

double operator*(const VectorData& lhs, const VectorData& rhs) {
  if ((lhs.storage_->DimensionCount() == 0) ||
      (rhs.storage_->DimensionCount() == 0)) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  if (lhs.storage_->DimensionCount() != rhs.storage_->DimensionCount()) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  double dot_product = 0.0;
  size_t lhs_index = 0;
  size_t rhs_index = 0;
  while (lhs_index < lhs.storage_->GetSize() &&
         rhs_index < rhs.storage_->GetSize()) {
    if (lhs.storage_->GetPointAt(lhs_index) ==
        rhs.storage_->GetPointAt(rhs_index)) {
      dot_product += double{lhs.storage_->values()[lhs_index]} *
                     rhs.storage_->values()[rhs_index];
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
  if ((storage_->DimensionCount() == 0) ||
      (other.storage_->DimensionCount() == 0)) {
    return;
  }

  if (storage_->DimensionCount() != other.storage_->DimensionCount()) {
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
  if (storage_->DimensionCount() == 0) {
    return;
  }

  size_t index = 0;
  while (index < storage_->GetSize()) {
    storage_->values()[index] /= scalar;
    ++index;
  }
}

double VectorData::GetMagnitude() const {
  return sqrt(std::accumulate(storage_->values().cbegin(),
                              storage_->values().cend(), 0.0,
                              [](const double& lhs, float v) -> double {
                                return lhs + double{v} * v;
                              }));
}

void VectorData::Normalize() {
  const auto vector_length = GetMagnitude();
  if (vector_length > kMinimumVectorLength) {
    for (float& entry : storage_->values()) {
      entry /= static_cast<float>(vector_length);
    }
  }
}

float VectorData::ComputeSimilarity(const VectorData& other) const {
  DCHECK(this->GetDimensionCount() == other.GetDimensionCount());
  return static_cast<float>((*this * other) /
                            (GetMagnitude() * other.GetMagnitude()));
}

int VectorData::GetDimensionCount() const {
  return storage_->DimensionCount();
}

int VectorData::GetNonZeroElementCount() const {
  if (storage_->DimensionCount() == 0) {
    return 0;
  }

  return base::ranges::count_if(storage_->values(),
                                [](float value) { return value != 0; });
}

std::vector<float> VectorData::GetAsFloatVector() const {
  return storage_->values();
}

std::string VectorData::GetVectorAsString() const {
  if (storage_->DimensionCount() == 0) {
    return {};
  }

  size_t storage_size = storage_->GetSize();
  if (storage_size == 0) {
    return {};
  }

  std::vector<std::string> vector_as_string;
  for (size_t index = 0; index < storage_size; ++index) {
    vector_as_string.push_back(base::NumberToString(storage_->values()[index]));
  }

  return base::JoinString(vector_as_string, " ");
}

}  // namespace brave_ads::ml
