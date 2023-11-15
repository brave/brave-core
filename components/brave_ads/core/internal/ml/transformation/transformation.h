/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_TRANSFORMATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_TRANSFORMATION_H_

#include <memory>

#include "brave/components/brave_ads/core/internal/ml/transformation/transformation_types.h"

namespace brave_ads::ml {

class Data;

class Transformation {
 public:
  explicit Transformation(TransformationType type);

  virtual ~Transformation();

  TransformationType GetType() const;

  virtual std::unique_ptr<Data> Apply(
      const std::unique_ptr<Data>& input_data) const = 0;

 protected:
  const TransformationType type_;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_TRANSFORMATION_H_
