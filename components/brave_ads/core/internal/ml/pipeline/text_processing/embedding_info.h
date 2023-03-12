/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

namespace ads::ml::pipeline {

struct TextEmbeddingInfo final {
  TextEmbeddingInfo();

  TextEmbeddingInfo(const TextEmbeddingInfo& other);
  TextEmbeddingInfo& operator=(const TextEmbeddingInfo& other);

  TextEmbeddingInfo(TextEmbeddingInfo&& other) noexcept;
  TextEmbeddingInfo& operator=(TextEmbeddingInfo&& other) noexcept;

  ~TextEmbeddingInfo();

  std::string text;
  std::string hashed_text_base64;
  std::string locale;
  VectorData embedding;
};

}  // namespace ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_INFO_H_
