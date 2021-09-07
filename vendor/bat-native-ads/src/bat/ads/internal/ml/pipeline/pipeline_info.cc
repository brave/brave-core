/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/pipeline_info.h"

#include "bat/ads/internal/ml/ml_transformation_util.h"
#include "bat/ads/internal/ml/transformation/transformation.h"

namespace ads {
namespace ml {
namespace pipeline {

PipelineInfo::PipelineInfo() = default;

PipelineInfo::PipelineInfo(const PipelineInfo& pinfo) {
  version = pinfo.version;
  timestamp = pinfo.timestamp;
  locale = pinfo.locale;
  linear_model = pinfo.linear_model;
  transformations = GetTransformationVectorDeepCopy(pinfo.transformations);
}

PipelineInfo::~PipelineInfo() = default;

PipelineInfo::PipelineInfo(const int& version,
                           const std::string& timestamp,
                           const std::string& locale,
                           const TransformationVector& new_transformations,
                           const model::Linear& linear_model)
    : version(version),
      timestamp(timestamp),
      locale(locale),
      linear_model(linear_model) {
  transformations = GetTransformationVectorDeepCopy(new_transformations);
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
