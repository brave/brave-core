/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <cstdint>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "bat/ads/internal/common/crypto/crypto_util.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/pipeline/embedding_pipeline_info.h"
#include "bat/ads/internal/ml/pipeline/embedding_pipeline_value_util.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_info.h"

namespace ads::ml::pipeline {

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

bool EmbeddingProcessing::IsInitialized() const {
  return is_initialized_;
}

bool EmbeddingProcessing::SetEmbeddingPipeline(base::Value resource_value) {
  const base::Value::Dict* const value = resource_value.GetIfDict();
  if (!value) {
    return is_initialized_;
  }

  const absl::optional<EmbeddingPipelineInfo> embedding_pipeline =
      EmbeddingPipelineFromValue(*value);
  if (!embedding_pipeline) {
    is_initialized_ = false;
  } else {
    embedding_pipeline_ = *embedding_pipeline;
    is_initialized_ = true;
  }

  return is_initialized_;
}

TextEmbeddingInfo EmbeddingProcessing::EmbedText(
    const std::string& text) const {
  if (!IsInitialized()) {
    return {};
  }

  if (text.empty()) {
    return {};
  }

  const std::vector<float> embedding_zeroed(embedding_pipeline_.dimension,
                                            0.0F);
  TextEmbeddingInfo text_embedding;
  text_embedding.embedding = VectorData(embedding_zeroed);
  text_embedding.locale = embedding_pipeline_.locale;

  const std::vector<std::string> tokens = base::SplitString(
      text, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  std::vector<std::string> in_vocab_tokens;

  for (const auto& token : tokens) {
    const auto iter = embedding_pipeline_.embeddings.find(token);
    if (iter == embedding_pipeline_.embeddings.end()) {
      BLOG(9,
           token << " - text embedding token not found in resource vocabulary");
      continue;
    }

    BLOG(9, token << " - text embedding token found in resource vocabulary");
    const VectorData& token_embedding_vector_data = iter->second;
    text_embedding.embedding.AddElementWise(token_embedding_vector_data);
    in_vocab_tokens.push_back(token);
  }

  if (in_vocab_tokens.empty()) {
    return text_embedding;
  }

  const std::string in_vocab_text = base::JoinString(in_vocab_tokens, " ");
  const std::vector<uint8_t> in_vocab_sha256 = crypto::Sha256(in_vocab_text);
  text_embedding.hashed_text_base64 = base::Base64Encode(in_vocab_sha256);

  const auto scalar = static_cast<float>(in_vocab_tokens.size());
  text_embedding.embedding.DivideByScalar(scalar);
  return text_embedding;
}

}  // namespace ads::ml::pipeline
