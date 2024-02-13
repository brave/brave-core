/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"

#include <utility>

namespace brave_ads::ml::pipeline {

PipelineInfo::PipelineInfo(std::string locale,
                           TransformationVector transformations,
                           std::optional<LinearModel> linear_model,
                           std::optional<NeuralModel> neural_model)
    : locale(std::move(locale)),
      transformations(std::move(transformations)),
      linear_model(std::move(linear_model)),
      neural_model(std::move(neural_model)) {}

PipelineInfo::PipelineInfo() = default;

PipelineInfo::PipelineInfo(PipelineInfo&& other) noexcept = default;

PipelineInfo& PipelineInfo::operator=(PipelineInfo&& other) noexcept = default;

PipelineInfo::~PipelineInfo() = default;

}  // namespace brave_ads::ml::pipeline
