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

namespace brave_ads::ml {

MappedTokensTransformation::MappedTokensTransformation()
    : Transformation(TransformationType::kMappedTokens) {}

MappedTokensTransformation::MappedTokensTransformation(
    int vector_dimension,
    std::map<std::string, std::vector<int>> token_categories_mapping)
    : Transformation(TransformationType::kMappedTokens) {
  vector_dimension_ = vector_dimension;
  token_categories_mapping_ = std::move(token_categories_mapping);
}

MappedTokensTransformation::MappedTokensTransformation(
    MappedTokensTransformation&& mapped_tokens) noexcept = default;

MappedTokensTransformation::~MappedTokensTransformation() = default;

// static
std::vector<std::string> MappedTokensTransformation::GetTokensFromText(
    const std::unique_ptr<Data>& input_data) {
  auto* text_data = static_cast<TextData*>(input_data.get());
  std::string text = text_data->GetText();
  std::vector<std::string> tokens = base::SplitString(
      text, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return tokens;
}

std::map<unsigned, double> MappedTokensTransformation::GetCategoryFrequencies(
    std::vector<std::string> tokens) const {
  std::map<unsigned, double> frequencies;

  size_t token_max_length = 5;
  size_t tokens_length = tokens.size();
  for (size_t i = 0; i < tokens_length; i++) {
    std::string token_candidate;
    for (size_t j = 0; j < token_max_length; j++) {
      if (i + j >= tokens_length) {
        continue;
      }
      std::string token_separator = j > 0 ? "-" : "";
      std::string token_candidate_addition = token_separator + tokens[i + j];
      token_candidate += token_candidate_addition;

      const auto iter = token_categories_mapping_.find(token_candidate);
      if (iter == token_categories_mapping_.end()) {
        continue;
      }

      BLOG(9, token_candidate << " - token found in category mapping");
      std::vector<int> category_indexes = iter->second;
      for (const auto& category_index : category_indexes) {
        ++frequencies[category_index];
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

  std::vector<std::string> tokens = GetTokensFromText(input_data);
  std::map<unsigned, double> frequencies = GetCategoryFrequencies(tokens);
  return std::make_unique<VectorData>(vector_dimension_, frequencies);
}

}  // namespace brave_ads::ml
