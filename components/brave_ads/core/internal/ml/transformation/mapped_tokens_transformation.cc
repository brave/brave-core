/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation.h"

#include <map>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hash_vectorizer.h"

namespace brave_ads::ml {

MappedTokensTransformation::MappedTokensTransformation()
    : Transformation(TransformationType::kMappedTokens) {
  vector_dimension_ = 0;
  token_categories_mapping_ = {};
}

MappedTokensTransformation::MappedTokensTransformation(
    int vector_dimension,
    std::map<std::string, std::vector<int>> token_categories_mapping)
    : Transformation(TransformationType::kMappedTokens) {
  vector_dimension_ = vector_dimension;
  token_categories_mapping_ = token_categories_mapping;
}

MappedTokensTransformation::MappedTokensTransformation(
    MappedTokensTransformation&& mapped_tokens) noexcept = default;

MappedTokensTransformation::~MappedTokensTransformation() = default;

std::unique_ptr<Data> MappedTokensTransformation::Apply(
    const std::unique_ptr<Data>& input_data) const {
  CHECK(input_data);

  if (input_data->GetType() != DataType::kText) {
    BLOG(0, "MappedTokensTransformation input not of type text");
    return {};
  }

  auto* text_data = static_cast<TextData*>(input_data.get());
  std::string text = text_data->GetText();
  std::vector<std::string> tokens = base::SplitString(
      text, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::map<unsigned, double> frequencies;
  size_t tokens_length = tokens.size();
  for (size_t i = 0; i < tokens_length; i++) {

    std::string token_candidate = "";
    for (size_t j = 0; j < 5; j++) {
      if (i + j >= tokens_length) {
        continue;
      }
      std::string token_separator = (j > 0) ? "-" : "";
      std::string token_candidate_addition = token_separator + tokens[i + j];
      token_candidate += token_candidate_addition;

      const auto iter = token_categories_mapping_.find(token_candidate);
      if (iter == token_categories_mapping_.end()) {
        BLOG(9, token_candidate << " - token not found in category mapping");
        continue;
      }

      BLOG(2, token_candidate);
      BLOG(9, token_candidate << " - token found in category mapping");
      std::vector<int> categories = iter->second;
      for (const auto& category_index : categories) {
        ++frequencies[category_index];
      }
    }
  }

  BLOG(2, "\n\ntoken mapping finished\n\n");
  return std::make_unique<VectorData>(vector_dimension_, frequencies);
}

}  // namespace brave_ads::ml
