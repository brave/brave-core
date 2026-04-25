/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_ML_ALIAS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_ML_ALIAS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/ml/transformation/transformation.h"

namespace brave_ads::ml {

using PredictionMap = std::map</*class*/ std::string, /*prediction*/ double>;
using TransformationPtr = std::unique_ptr<Transformation>;
using TransformationVector = std::vector<TransformationPtr>;

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_ML_ALIAS_H_
