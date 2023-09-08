/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_MAPPED_TOKENS_TRANSFORMATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_MAPPED_TOKENS_TRANSFORMATION_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/ml/transformation/transformation.h"

namespace brave_ads::ml {

class MappedTokensTransformation final : public Transformation {
 public:
  MappedTokensTransformation();
  MappedTokensTransformation(
      int vector_dimension,
      std::map<std::string, std::vector<int>> token_categories_mapping);

  MappedTokensTransformation(
      MappedTokensTransformation&& mapped_tokens) noexcept;
  MappedTokensTransformation& operator=(
      MappedTokensTransformation&& mapped_tokens) = delete;

  ~MappedTokensTransformation() override;

  explicit MappedTokensTransformation(const std::string& parameters);

  static std::vector<std::string> GetTokensFromText(
      const std::unique_ptr<Data>& input_data);

  std::map<unsigned, double> GetCategoryFrequencies(
      std::vector<std::string> tokens) const;

  std::unique_ptr<Data> Apply(
      const std::unique_ptr<Data>& input_data) const override;

 private:
  int vector_dimension_ = 0;
  std::map<std::string, std::vector<int>> token_categories_mapping_;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_MAPPED_TOKENS_TRANSFORMATION_H_
