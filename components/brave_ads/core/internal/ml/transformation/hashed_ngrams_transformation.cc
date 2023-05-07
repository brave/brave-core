/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"

#include <map>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hash_vectorizer.h"

namespace brave_ads::ml {

HashedNGramsTransformation::HashedNGramsTransformation()
    : Transformation(TransformationType::kHashedNGrams) {
  hash_vectorizer_ = std::make_unique<HashVectorizer>();
}

HashedNGramsTransformation::HashedNGramsTransformation(
    const int bucket_count,
    const std::vector<int>& subgrams)
    : Transformation(TransformationType::kHashedNGrams) {
  hash_vectorizer_ = std::make_unique<HashVectorizer>(bucket_count, subgrams);
}

HashedNGramsTransformation::HashedNGramsTransformation(
    HashedNGramsTransformation&& hashed_ngrams) noexcept = default;

HashedNGramsTransformation::~HashedNGramsTransformation() = default;

std::unique_ptr<Data> HashedNGramsTransformation::Apply(
    const std::unique_ptr<Data>& input_data) const {
  CHECK(input_data->GetType() == DataType::kText);

  auto* text_data = static_cast<TextData*>(input_data.get());

  const std::map<unsigned, double> frequences =
      hash_vectorizer_->GetFrequencies(text_data->GetText());
  const int dimension_count = hash_vectorizer_->GetBucketCount();

  return std::make_unique<VectorData>(dimension_count, frequences);
}

}  // namespace brave_ads::ml
