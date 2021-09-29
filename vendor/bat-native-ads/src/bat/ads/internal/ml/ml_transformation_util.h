/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_ML_TRANSFORMATION_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_ML_TRANSFORMATION_UTIL_H_

#include "bat/ads/internal/ml/ml_aliases.h"

namespace ads {
namespace ml {

TransformationPtr GetTransformationCopy(
    const TransformationPtr& transformation_ptr);

TransformationVector GetTransformationVectorDeepCopy(
    const TransformationVector& transformation_vector);

}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_ML_TRANSFORMATION_UTIL_H_
