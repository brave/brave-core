/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/neural/neural_architecture_info.h"

#include <utility>

namespace brave_ads::ml {

NeuralArchitectureInfo::NeuralArchitectureInfo(
    std::vector<std::vector<VectorData>> matricies,
    std::vector<PostMatrixFunctionType> post_matrix_functions,
    std::vector<std::string> classes)
    : matricies(std::move(matricies)),
      post_matrix_functions(std::move(post_matrix_functions)),
      classes(std::move(classes)) {}

NeuralArchitectureInfo::NeuralArchitectureInfo() = default;

NeuralArchitectureInfo::NeuralArchitectureInfo(
    NeuralArchitectureInfo&& other) noexcept = default;

NeuralArchitectureInfo& NeuralArchitectureInfo::operator=(
    NeuralArchitectureInfo&& other) noexcept = default;

NeuralArchitectureInfo::~NeuralArchitectureInfo() = default;

}  // namespace brave_ads::ml
