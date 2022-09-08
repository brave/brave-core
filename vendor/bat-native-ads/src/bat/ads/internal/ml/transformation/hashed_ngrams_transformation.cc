/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/transformation/hashed_ngrams_transformation.h"

#include <map>

#include "base/check.h"
#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/transformation/hash_vectorizer.h"

namespace ads::ml {

HashedNGramsTransformation::HashedNGramsTransformation()
    : Transformation(TransformationType::kHashedNGrams) {
  hash_vectorizer = std::make_unique<HashVectorizer>();
}

HashedNGramsTransformation::HashedNGramsTransformation(
    HashedNGramsTransformation&& hashed_ngrams) noexcept = default;

HashedNGramsTransformation::~HashedNGramsTransformation() = default;

HashedNGramsTransformation::HashedNGramsTransformation(
    const int bucket_count,
    const std::vector<int>& subgrams)
    : Transformation(TransformationType::kHashedNGrams) {
  hash_vectorizer = std::make_unique<HashVectorizer>(bucket_count, subgrams);
}

std::unique_ptr<Data> HashedNGramsTransformation::Apply(
    const std::unique_ptr<Data>& input_data) const {
  DCHECK(input_data->GetType() == DataType::kText);

  auto* text_data = static_cast<TextData*>(input_data.get());

  std::map<unsigned, double> frequences =
      hash_vectorizer->GetFrequencies(text_data->GetText());
  int dimension_count = hash_vectorizer->GetBucketCount();

  return std::make_unique<VectorData>(dimension_count, frequences);
}

}  // namespace ads::ml
