/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "bat/ads/internal/base/crypto/crypto_util.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/strings/string_html_parse_util.h"
#include "bat/ads/internal/base/strings/string_strip_util.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/pipeline/pipeline_embedding_info.h"
#include "bat/ads/internal/ml/pipeline/pipeline_util.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_data.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace ml {
namespace pipeline {

// static
std::unique_ptr<EmbeddingProcessing> EmbeddingProcessing::CreateFromValue(
    base::Value resource_value,
    std::string* error_message) {
  DCHECK(error_message);

  auto text_processing = std::make_unique<EmbeddingProcessing>();
  if (!text_processing->FromValue(std::move(resource_value))) {
    *error_message = "Failed to parse text embedding pipeline JSON";
    return {};
  }

  return text_processing;
}

bool EmbeddingProcessing::IsInitialized() const {
  return is_initialized_;
}

EmbeddingProcessing::EmbeddingProcessing() = default;

EmbeddingProcessing::~EmbeddingProcessing() = default;

void EmbeddingProcessing::SetIsInitialized(bool is_initialized) {
  is_initialized_ = is_initialized;
}

void EmbeddingProcessing::SetEmbeddingPipeline(
    const PipelineEmbeddingInfo& info) {
  embedding_pipeline_.version = info.version;
  embedding_pipeline_.timestamp = info.timestamp;
  embedding_pipeline_.locale = info.locale;
  embedding_pipeline_.dim = info.dim;
  embedding_pipeline_.embeddings = info.embeddings;
}

bool EmbeddingProcessing::FromValue(base::Value resource_value) {
  absl::optional<PipelineEmbeddingInfo> pipeline_embedding =
      ParsePipelineEmbedding(std::move(resource_value));

  if (pipeline_embedding.has_value()) {
    SetEmbeddingPipeline(pipeline_embedding.value());
    SetIsInitialized(true);
  } else {
    SetIsInitialized(false);
  }

  return is_initialized_;
}

std::string EmbeddingProcessing::SanitizeText(const std::string& text,
                                              bool is_html) {
  std::string cleaned_text = text;
  if (is_html) {
    cleaned_text = ParseTagAttribute(cleaned_text, "og:title", "content");
  }
  cleaned_text = StripNonAlphaCharacters(cleaned_text);
  std::transform(cleaned_text.begin(), cleaned_text.end(), cleaned_text.begin(),
                 ::tolower);
  return cleaned_text;
}

TextEmbeddingData EmbeddingProcessing::EmbedText(
    const std::string& text) const {
  std::vector<float> embedding_initialize(embedding_pipeline_.dim, 0.0f);
  VectorData embedding_vector = VectorData(embedding_initialize);
  TextEmbeddingData embedding_data;
  embedding_data.embedding = embedding_vector;

  if (!IsInitialized()) {
    return embedding_data;
  }

  const std::vector<std::string> tokens = base::SplitString(
      text, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::vector<std::string> in_vocab_tokens;
  int n_tokens = 0;
  for (const auto& token : tokens) {
    const auto iter = embedding_pipeline_.embeddings.find(token);
    if (iter != embedding_pipeline_.embeddings.end()) {
      BLOG(9, token << " - token found");
      embedding_data.embedding.AddElementWise(iter->second);
      in_vocab_tokens.push_back(token);
      n_tokens += 1;
    } else {
      BLOG(9, token);
    }
  }

  if (n_tokens == 0) {
    return embedding_data;
  }

  std::string in_vocab_text = base::JoinString(in_vocab_tokens, " ");
  const std::vector<uint8_t> sha256_hash = security::Sha256(in_vocab_text);
  const std::string hashed_text = base::Base64Encode(sha256_hash);
  embedding_data.text_hashed = hashed_text;

  float scalar = static_cast<float>(n_tokens);
  embedding_data.embedding.DivideByScalar(scalar);
  return embedding_data;
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
