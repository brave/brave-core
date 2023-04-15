/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_PIPELINE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_PIPELINE_UTIL_H_

#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::ml::pipeline {

struct PipelineInfo;

absl::optional<PipelineInfo> ParsePipelineValue(base::Value::Dict dict);

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_PIPELINE_UTIL_H_
