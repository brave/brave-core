/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_EMBEDDING_PIPELINE_VALUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_EMBEDDING_PIPELINE_VALUE_UTIL_H_

#include <optional>

#include "base/values.h"

namespace brave_ads::ml::pipeline {

struct EmbeddingPipelineInfo;

std::optional<EmbeddingPipelineInfo> EmbeddingPipelineFromValue(
    const base::Value::Dict& dict);

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_EMBEDDING_PIPELINE_VALUE_UTIL_H_
