/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_LINEAR_PIPELINE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_LINEAR_PIPELINE_UTIL_H_

#include <optional>

namespace brave_ads::ml::pipeline {

struct PipelineInfo;

std::optional<PipelineInfo> LoadLinearPipeline(const uint8_t* data,
                                               size_t length);

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_LINEAR_PIPELINE_UTIL_H_
