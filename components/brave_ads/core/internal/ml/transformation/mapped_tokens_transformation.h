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

#include "base/allocator/partition_allocator/pointers/raw_ptr.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/transformation.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::text_classification::flat {
struct MappedTokenTransformation;
}  // namespace brave_ads::text_classification::flat

namespace brave_ads::ml {

class MappedTokensTransformation final : public Transformation {
 public:
  explicit MappedTokensTransformation(
      const text_classification::flat::MappedTokenTransformation*
          mapped_token_transformation);

  MappedTokensTransformation(
      MappedTokensTransformation&& mapped_tokens) noexcept;
  MappedTokensTransformation& operator=(
      MappedTokensTransformation&& mapped_tokens) = delete;

  ~MappedTokensTransformation() override;

  explicit MappedTokensTransformation(const std::string& parameters);

  static std::vector<std::string> GetWordsFromText(
      const std::unique_ptr<Data>& input_data);

  absl::optional<std::map<uint32_t, double>> GetCategoryFrequencies(
      const std::vector<std::string>& words) const;

  std::unique_ptr<Data> Apply(
      const std::unique_ptr<Data>& input_data) const override;

 private:
  raw_ptr<const text_classification::flat::MappedTokenTransformation>
      mapped_token_transformation_;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_MAPPED_TOKENS_TRANSFORMATION_H_
