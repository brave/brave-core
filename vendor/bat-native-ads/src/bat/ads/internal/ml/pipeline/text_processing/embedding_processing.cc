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
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_info.h"

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

  is_initialized_ = embedding_pipeline_.FromValue(*value);

  return is_initialized_;
}

TextEmbeddingInfo EmbeddingProcessing::EmbedText(
    const std::string& text) const {
  std::vector<float> embedding_zeroed(embedding_pipeline_.dim, 0.0F);
  VectorData embedding_vector_data = VectorData(embedding_zeroed);
  TextEmbeddingInfo text_embedding;
  text_embedding.embedding = embedding_vector_data;

  if (!IsInitialized()) {
    return text_embedding;
  }

  if (text.empty()) {
    return text_embedding;
  }

  const std::vector<std::string> tokens = base::SplitString(
      text, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::vector<std::string> in_vocab_tokens;
  int n_tokens = 0;
  for (const auto& token : tokens) {
    const auto iter = embedding_pipeline_.embeddings.find(token);
    if (iter != embedding_pipeline_.embeddings.end()) {
      BLOG(9, token << " - text embedding token found in resource vocabulary");
      const VectorData& token_embedding_vector_data = iter->second;
      text_embedding.embedding.AddElementWise(token_embedding_vector_data);
      in_vocab_tokens.push_back(token);
      n_tokens++;
    } else {
      BLOG(9,
           token << " - text embedding token not found in resource vocabulary");
    }
  }

  if (n_tokens == 0) {
    return text_embedding;
  }

  const std::string in_vocab_text = base::JoinString(in_vocab_tokens, " ");
  const std::vector<uint8_t> sha256_hash = security::Sha256(in_vocab_text);
  text_embedding.text_hashed = base::Base64Encode(sha256_hash);

  const auto scalar = static_cast<float>(n_tokens);
  text_embedding.embedding.DivideByScalar(scalar);
  return text_embedding;
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
