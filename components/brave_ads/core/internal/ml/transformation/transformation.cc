/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/transformation.h"

namespace brave_ads::ml {

Transformation::Transformation(TransformationType type) : type_(type) {}

Transformation::~Transformation() = default;

TransformationType Transformation::GetType() const {
  return type_;
}

}  // namespace brave_ads::ml
