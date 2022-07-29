/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_DATA_H_

#include <string>

#include "bat/ads/internal/ml/data/vector_data.h"

namespace ads {
namespace ml {
namespace pipeline {

struct TextEmbeddingData final {
  TextEmbeddingData();
  TextEmbeddingData(const TextEmbeddingData& data);
  TextEmbeddingData& operator=(const TextEmbeddingData& data);
  ~TextEmbeddingData();

  std::string text;
  std::string text_hashed;
  VectorData embedding;
};

}  // namespace pipeline
}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_DATA_H_
