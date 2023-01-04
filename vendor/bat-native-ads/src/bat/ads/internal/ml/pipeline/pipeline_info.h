/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_INFO_H_

#include <string>

#include "bat/ads/internal/ml/ml_alias.h"
#include "bat/ads/internal/ml/model/linear/linear.h"

namespace ads::ml::pipeline {

struct PipelineInfo final {
  PipelineInfo();
  PipelineInfo(int version,
               std::string timestamp,
               std::string locale,
               TransformationVector transformations,
               model::Linear linear_model);

  PipelineInfo(PipelineInfo&& other) noexcept;
  PipelineInfo& operator=(PipelineInfo&& other) noexcept;

  ~PipelineInfo();

  int version;
  std::string timestamp;
  std::string locale;
  TransformationVector transformations;
  model::Linear linear_model;
};

}  // namespace ads::ml::pipeline

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_INFO_H_
