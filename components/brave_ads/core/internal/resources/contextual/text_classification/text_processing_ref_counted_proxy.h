/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_PROCESSING_REF_COUNTED_PROXY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_PROCESSING_REF_COUNTED_PROXY_H_

#include <memory>
#include <string>

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_alias.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/brave_ads/core/internal/resources/async/resource_ref_counted_proxy_base.h"

namespace brave_ads {

class TextProcessingRefCountedProxy final
    : public ResourceRefCounterProxyBase<ml::pipeline::TextProcessing> {
 public:
  TextProcessingRefCountedProxy();
  ~TextProcessingRefCountedProxy() override;

  TextClassificationProbabilityMap ClassifyPage(const std::string& text) const;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_PROCESSING_REF_COUNTED_PROXY_H_
