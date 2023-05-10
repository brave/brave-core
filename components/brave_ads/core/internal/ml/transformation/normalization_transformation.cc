/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/normalization_transformation.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

namespace brave_ads::ml {

NormalizationTransformation::NormalizationTransformation()
    : Transformation(TransformationType::kNormalization) {}

std::unique_ptr<Data> NormalizationTransformation::Apply(
    const std::unique_ptr<Data>& input_data) const {
  CHECK(input_data->GetType() == DataType::kVector);

  auto* vector_data = static_cast<VectorData*>(input_data.get());

  VectorData vector_data_copy = *vector_data;
  vector_data_copy.Normalize();
  return std::make_unique<VectorData>(std::move(vector_data_copy));
}

}  // namespace brave_ads::ml
