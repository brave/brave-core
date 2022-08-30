/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/pipeline_info.h"

#include <utility>

namespace ads {
namespace ml {
namespace pipeline {

PipelineInfo::PipelineInfo() = default;

PipelineInfo::PipelineInfo(PipelineInfo&& info) noexcept = default;

PipelineInfo& PipelineInfo::operator=(PipelineInfo&& info) noexcept = default;

PipelineInfo::~PipelineInfo() = default;

PipelineInfo::PipelineInfo(const int version,
                           const std::string& timestamp,
                           const std::string& locale,
                           TransformationVector new_transformations,
                           model::Linear linear_model)
    : version(version),
      timestamp(timestamp),
      locale(locale),
      linear_model(std::move(linear_model)) {
  transformations = std::move(new_transformations);
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
