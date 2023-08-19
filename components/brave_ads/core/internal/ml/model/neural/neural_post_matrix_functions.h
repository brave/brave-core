/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_POST_MATRIX_FUNCTIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_POST_MATRIX_FUNCTIONS_H_

namespace brave_ads::ml {

enum class PostMatrixFunctionType {
  kNone = 0,
  kSoftmax,
  kTanh,
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_NEURAL_POST_MATRIX_FUNCTIONS_H_
