/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "bat/ads/internal/base/crypto/crypto_util.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/pipeline/pipeline_embedding_info.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_data.h"

namespace ads {
namespace ml {
namespace pipeline {

// static
std::unique_ptr<EmbeddingProcessing> EmbeddingProcessing::CreateFromValue(
    base::Value resource_value,
    std::string* error_message) {
  DCHECK(error_message);

  auto embedding_processing = std::make_unique<EmbeddingProcessing>();
  if (!embedding_processing->SetEmbeddingPipeline(std::move(resource_value))) {
    *error_message = "Failed to parse embedding pipeline JSON";
    return nullptr;
  }

  return embedding_processing;
}

EmbeddingProcessing::EmbeddingProcessing() = default;

EmbeddingProcessing::~EmbeddingProcessing() = default;

bool EmbeddingProcessing::IsInitialized() const {
  return is_initialized_;
}

bool EmbeddingProcessing::SetEmbeddingPipeline(base::Value resource_value) {
  base::Value::Dict* value = resource_value.GetIfDict();
  if (!value) {
    return is_initialized_;
  }

  const bool success = embedding_pipeline_.FromValue(*value);
  if (success) {
    is_initialized_ = true;
  } else {
    is_initialized_ = false;
  }

  return is_initialized_;
}

bool EmbeddingProcessing::SetEmbeddingPipelineForTesting(
    const int version,
    const base::Time time,
    const std::string& locale,
    const int dim,
    const std::map<std::string, VectorData>& embeddings) {
  embedding_pipeline_.version = version;
  embedding_pipeline_.time = time;
  embedding_pipeline_.locale = locale;
  embedding_pipeline_.dim = dim;
  embedding_pipeline_.embeddings = embeddings;
  is_initialized_ = true;

  return is_initialized_;
}

TextEmbeddingData EmbeddingProcessing::EmbedText(
    const std::string& text) const {
  std::vector<float> embedding_zeroed(embedding_pipeline_.dim, 0.0f);
  VectorData embedding_vector = VectorData(embedding_zeroed);
  TextEmbeddingData embedding_data;
  embedding_data.embedding = embedding_vector;

  if (!IsInitialized()) {
    return embedding_data;
  }

  if (text.empty()) {
    return embedding_data;
  }

  const std::vector<std::string> tokens = base::SplitString(
      text, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::vector<std::string> in_vocab_tokens;
  int n_tokens = 0;
  for (const auto& token : tokens) {
    const auto iter = embedding_pipeline_.embeddings.find(token);
    if (iter != embedding_pipeline_.embeddings.end()) {
      BLOG(9, token << " - text token found in vocabulary");
      const VectorData& token_embedding = iter->second;
      embedding_data.embedding.AddElementWise(token_embedding);
      in_vocab_tokens.push_back(token);
      n_tokens++;
    } else {
      BLOG(9, token << " - text token not found in vocabulary");
    }
  }

  if (n_tokens == 0) {
    return embedding_data;
  }

  const std::string in_vocab_text = base::JoinString(in_vocab_tokens, " ");
  const std::vector<uint8_t> sha256_hash = security::Sha256(in_vocab_text);
  embedding_data.text_hashed = base::Base64Encode(sha256_hash);

  const float scalar = static_cast<float>(n_tokens);
  embedding_data.embedding.DivideByScalar(scalar);
  return embedding_data;
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
