/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_PIPELINE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_PIPELINE_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"

namespace brave_ads::ml::pipeline {

struct PipelineInfo final {
  PipelineInfo();
  PipelineInfo(int version,
               std::string timestamp,
               std::string locale,
               TransformationVector transformations,
               LinearModel linear_model);

  PipelineInfo(PipelineInfo&&) noexcept;
  PipelineInfo& operator=(PipelineInfo&&) noexcept;

  ~PipelineInfo();

  int version;
  std::string timestamp;
  std::string locale;
  TransformationVector transformations;
  LinearModel linear_model;
};

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_PIPELINE_INFO_H_
