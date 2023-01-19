/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_UTIL_H_

#include "absl/types/optional.h"

namespace base {
class Value;
}  // namespace base

namespace ads::ml::pipeline {

struct PipelineInfo;

absl::optional<PipelineInfo> ParsePipelineValue(base::Value resource_value);

}  // namespace ads::ml::pipeline

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_UTIL_H_
