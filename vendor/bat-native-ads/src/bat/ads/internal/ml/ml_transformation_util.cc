/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/ml_transformation_util.h"

#include <memory>

#include "base/notreached.h"
#include "bat/ads/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "bat/ads/internal/ml/transformation/lowercase_transformation.h"
#include "bat/ads/internal/ml/transformation/normalization_transformation.h"
#include "bat/ads/internal/ml/transformation/transformation.h"

namespace ads {
namespace ml {

// The function should always return unique_ptr to transformation copy.
// NOTREACHED() is used to protect from handling unknown transformation types
TransformationPtr GetTransformationCopy(
    const TransformationPtr& transformation_ptr) {
  switch (transformation_ptr->GetType()) {
    case TransformationType::LOWERCASE: {
      LowercaseTransformation* lowercase_ptr =
          static_cast<LowercaseTransformation*>(transformation_ptr.get());
      LowercaseTransformation lowercase_copy = *lowercase_ptr;
      return std::make_unique<LowercaseTransformation>(lowercase_copy);
    }
    case TransformationType::HASHED_NGRAMS: {
      HashedNGramsTransformation* hashed_n_grams_ptr =
          static_cast<HashedNGramsTransformation*>(transformation_ptr.get());
      HashedNGramsTransformation hashed_n_grams_ptr_copy = *hashed_n_grams_ptr;
      return std::make_unique<HashedNGramsTransformation>(
          hashed_n_grams_ptr_copy);
    }
    case TransformationType::NORMALIZATION: {
      NormalizationTransformation* normalization_ptr =
          static_cast<NormalizationTransformation*>(transformation_ptr.get());
      NormalizationTransformation normalization_copy = *normalization_ptr;
      return std::make_unique<NormalizationTransformation>(normalization_copy);
    }
    default: {
      NOTREACHED();
      return TransformationPtr(nullptr);
    }
  }
}

TransformationVector GetTransformationVectorDeepCopy(
    const TransformationVector& transformation_vector) {
  TransformationVector transformation_vector_copy;
  for (const TransformationPtr& transformation : transformation_vector) {
    transformation_vector_copy.push_back(GetTransformationCopy(transformation));
  }
  return transformation_vector_copy;
}

}  // namespace ml
}  // namespace ads
