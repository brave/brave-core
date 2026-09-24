/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/data/data.h"

namespace brave_ads::ml {

Data::Data(DataType type) : type_(type) {}

Data::~Data() = default;

DataType Data::GetType() const {
  return type_;
}

}  // namespace brave_ads::ml
