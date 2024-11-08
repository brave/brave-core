/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation.h"

#include <cstdint>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_transformation_generated.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

namespace brave_ads::ml {

namespace {

std::vector<std::string> GetWordsFromText(const TextData& text_data) {
  const std::string& text = text_data.GetText();
  std::vector<std::string> words = base::SplitString(
      text, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return words;
}

}  // namespace

MappedTokensTransformation::MappedTokensTransformation(
    const neural_text_classification::flat::MappedTokenTransformation&
        mapped_token_transformation)
    : Transformation(TransformationType::kMappedTokens),
      mapped_token_transformation_(mapped_token_transformation) {}

MappedTokensTransformation::~MappedTokensTransformation() = default;

std::optional<std::map<uint32_t, double>>
MappedTokensTransformation::GetCategoryFrequencies(
    const std::vector<std::string>& words) const {
  const auto* const token_categories = mapped_token_transformation_->mapping();
  if (!token_categories) {
    return std::nullopt;
  }

  std::map<uint32_t, double> frequencies;

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

      const auto* const iter =
          token_categories->LookupByKey(token_candidate.c_str());
      if (!iter || !iter->segment_indices()) {
        continue;
      }

      for (const uint16_t index : *iter->segment_indices()) {
        ++frequencies[static_cast<uint32_t>(index)];
      }
    }
  }
  return frequencies;
}

std::unique_ptr<Data> MappedTokensTransformation::Apply(
    const std::unique_ptr<Data>& input_data) const {
  CHECK(input_data);

  if (input_data->GetType() != DataType::kText) {
    return {};
  }

  const auto* const text_data = static_cast<TextData*>(input_data.get());
  CHECK(text_data);

  std::vector<std::string> words = GetWordsFromText(*text_data);
  std::optional<std::map<uint32_t, double>> frequencies =
      GetCategoryFrequencies(words);
  if (!frequencies) {
    return {};
  }

  return std::make_unique<VectorData>(mapped_token_transformation_->dimension(),
                                      *frequencies);
}

}  // namespace brave_ads::ml
