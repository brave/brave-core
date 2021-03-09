/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/transformation/transformation.h"

namespace ads {
namespace ml {

Transformation::Transformation(const TransformationType& type) : type_(type) {}

Transformation::Transformation(const Transformation& t) = default;

Transformation::~Transformation() = default;

TransformationType Transformation::GetType() const {
  return type_;
}

}  // namespace ml
}  // namespace ads
