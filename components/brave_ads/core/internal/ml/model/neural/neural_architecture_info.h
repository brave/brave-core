/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_ARCHITECTURE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_ARCHITECTURE_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/model/neural/neural_post_matrix_functions.h"

namespace brave_ads::ml {

struct NeuralArchitectureInfo final {
  NeuralArchitectureInfo();
  NeuralArchitectureInfo(
      std::vector<std::vector<VectorData>> matricies,
      std::vector<PostMatrixFunctionType> post_matrix_functions,
      std::vector<std::string> classes);

  NeuralArchitectureInfo(NeuralArchitectureInfo&&) noexcept;
  NeuralArchitectureInfo& operator=(NeuralArchitectureInfo&&) noexcept;

  ~NeuralArchitectureInfo();

  std::vector<std::vector<VectorData>> matricies;
  std::vector<PostMatrixFunctionType> post_matrix_functions;
  std::vector<std::string> classes;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_MODEL_NEURAL_ARCHITECTURE_INFO_H_
