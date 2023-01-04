/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_VECTOR_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_VECTOR_DATA_H_

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ads/internal/ml/data/data.h"

namespace ads::ml {

class VectorData final : public Data {
 public:
  VectorData();

  // Make a "dense" DataVector with points 0..n-1 (n = data.size()):
  // ({0, data[0]}, {1, data[0]}, .., {n-1, data[n-1]}}
  explicit VectorData(std::vector<float> data);

  // Make a "sparse" DataVector using points from |data|.
  // double is used for backward compatibility with the current code.
  VectorData(int dimension_count, const std::map<uint32_t, double>& data);

  // Explicit copy assignment && move operators is required because the class
  // inherits const member type_ that cannot be copied by default
  VectorData(const VectorData& vector_data);
  VectorData& operator=(const VectorData& vector_data);

  // Explicit copy assignment && move operators is required because the class
  // inherits const member type_ that cannot be copied by default
  VectorData(VectorData&& vector_data) noexcept;
  VectorData& operator=(VectorData&& vector_data) noexcept;

  ~VectorData() override;

  // Mathematical vector operations
  friend double operator*(const VectorData& lhs, const VectorData& rhs);
  void AddElementWise(const VectorData& v_add);
  void DivideByScalar(float scalar);
  void Normalize();

  int GetDimensionCount() const;
  int GetNonZeroElementCount() const;

  const std::vector<float>& GetValuesForTesting() const;
  std::string GetVectorAsString() const;

 private:
  std::unique_ptr<class VectorDataStorage> storage_;
};

}  // namespace ads::ml

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_DATA_VECTOR_DATA_H_
