/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_EMBEDDING_PROCESSING_REF_COUNTED_PROXY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_EMBEDDING_PROCESSING_REF_COUNTED_PROXY_H_

#include <memory>
#include <string>

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_processing.h"
#include "brave/components/brave_ads/core/internal/resources/async/resource_ref_counted_proxy_base.h"

namespace brave_ads {

namespace ml::pipeline {
struct TextEmbeddingInfo;
}  // namespace ml::pipeline

class EmbeddingProcessingRefCountedProxy final
    : public ResourceRefCounterProxyBase<ml::pipeline::EmbeddingProcessing> {
 public:
  EmbeddingProcessingRefCountedProxy();
  ~EmbeddingProcessingRefCountedProxy() override;

  ml::pipeline::TextEmbeddingInfo EmbedText(const std::string& text) const;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_EMBEDDING_PROCESSING_REF_COUNTED_PROXY_H_
