/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_TRANSFORMATION_TRANSFORMATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_TRANSFORMATION_TRANSFORMATION_H_

#include <memory>

#include "bat/ads/internal/ml/data/data.h"

namespace ads {
namespace ml {

enum class TransformationType {
  LOWERCASE = 0,
  HASHED_NGRAMS = 1,
  NORMALIZATION = 2
};

class Transformation {
 public:
  explicit Transformation(const TransformationType& type);

  Transformation(const Transformation& t);

  virtual ~Transformation();

  TransformationType GetType() const;

  virtual std::unique_ptr<Data> Apply(
      const std::unique_ptr<Data>& input_data) const = 0;

 protected:
  const TransformationType type_;
};

}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_TRANSFORMATION_TRANSFORMATION_H_
