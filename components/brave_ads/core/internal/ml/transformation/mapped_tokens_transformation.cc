/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation.h"

#include <map>
#include <utility>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation_util.h"

namespace brave_ads::ml {

MappedTokensTransformation::MappedTokensTransformation()
    : Transformation(TransformationType::kMappedTokens) {}

MappedTokensTransformation::MappedTokensTransformation(
    int vector_dimension,
    std::map<std::string, std::vector<int>> huffman_coding_mapping,
    std::map<std::basic_string<unsigned char>, std::vector<unsigned char>>
        token_categories_mapping)
    : Transformation(TransformationType::kMappedTokens) {
  vector_dimension_ = vector_dimension;
  huffman_coding_mapping_ = std::move(huffman_coding_mapping);
  token_categories_mapping_ = std::move(token_categories_mapping);
}

MappedTokensTransformation::MappedTokensTransformation(
    MappedTokensTransformation&& mapped_tokens) noexcept = default;

MappedTokensTransformation::~MappedTokensTransformation() = default;

// static
std::vector<std::string> MappedTokensTransformation::GetWordsFromText(
    const std::unique_ptr<Data>& input_data) {
  auto* text_data = static_cast<TextData*>(input_data.get());
  std::string text = text_data->GetText();
  std::vector<std::string> words = base::SplitString(
      text, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return words;
}

std::map<unsigned, double> MappedTokensTransformation::GetCategoryFrequencies(
    std::vector<std::string> words) const {
  std::map<unsigned, double> frequencies;

  size_t token_max_length = 5;
  size_t words_length = words.size();
  for (size_t i = 0; i < words_length; i++) {
    std::string token_candidate;
    for (size_t j = 0; j < token_max_length; j++) {
      if (i + j >= words_length) {
        continue;
      }
      std::string token_separator = j > 0 ? "-" : "";
      std::string token_candidate_addition = token_separator + words[i + j];
      token_candidate += token_candidate_addition;

      absl::optional<std::basic_string<unsigned char>>
          compressed_token_candidate =
              CompressToken(token_candidate, huffman_coding_mapping_);
      if (!compressed_token_candidate) {
        break;
      }

      const auto iter =
          token_categories_mapping_.find(*compressed_token_candidate);
      if (iter == token_categories_mapping_.end()) {
        continue;
      }

      BLOG(9, token_candidate << " - token found in category mapping");
      const std::vector<unsigned char>& category_indexes = iter->second;
      for (const auto& category_index : category_indexes) {
        ++frequencies[static_cast<int>(category_index)];
      }
    }
  }
  return frequencies;
}

std::unique_ptr<Data> MappedTokensTransformation::Apply(
    const std::unique_ptr<Data>& input_data) const {
  CHECK(input_data);

  if (input_data->GetType() != DataType::kText) {
    BLOG(0, "MappedTokensTransformation input not of type text");
    return {};
  }

  std::vector<std::string> words = GetWordsFromText(input_data);
  std::map<unsigned, double> frequencies = GetCategoryFrequencies(words);
  return std::make_unique<VectorData>(vector_dimension_, frequencies);
}

}  // namespace brave_ads::ml
