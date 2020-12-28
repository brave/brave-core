/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_VECTOR_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_VECTOR_DATA_H_

#include <cstdint>
#include <map>
#include <vector>

#include "bat/ads/internal/ml/data/data.h"
#include "bat/ads/internal/ml/data/vector_data_aliases.h"

namespace ads {
namespace ml {

class VectorData : public Data {
 public:
  VectorData();

  VectorData(const VectorData& vector_data);

  explicit VectorData(const std::vector<double>& data);

  // Explicit copy assignment operator is required because the class
  // inherits const member type_ that cannot be copied by default
  VectorData& operator=(const VectorData& vector_data);

  VectorData(const int dimension_count, const std::map<uint32_t, double>& data);

  ~VectorData() override;

  friend double operator*(const VectorData& lhs, const VectorData& rhs);

  void Normalize();

  int GetDimensionCount() const;

  std::vector<SparseVectorElement> GetRawData() const;

 private:
  int dimension_count_;
  std::vector<SparseVectorElement> data_;
};

}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_VECTOR_DATA_H_
