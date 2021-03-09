/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_TRANSFORMATION_HASHED_NGRAMS_TRANSFORMATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_TRANSFORMATION_HASHED_NGRAMS_TRANSFORMATION_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ads/internal/ml/transformation/transformation.h"

namespace ads {
namespace ml {

class HashVectorizer;

class HashedNGramsTransformation : public Transformation {
 public:
  HashedNGramsTransformation();

  HashedNGramsTransformation(const HashedNGramsTransformation& hashed_ngrams);

  HashedNGramsTransformation(const int bucket_count,
                             const std::vector<int>& subgrams);

  ~HashedNGramsTransformation() override;

  explicit HashedNGramsTransformation(const std::string& parameters);

  std::unique_ptr<Data> Apply(
      const std::unique_ptr<Data>& input_data) const override;

 private:
  std::unique_ptr<HashVectorizer> hash_vectorizer;
};

}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_TRANSFORMATION_HASHED_NGRAMS_TRANSFORMATION_H_
