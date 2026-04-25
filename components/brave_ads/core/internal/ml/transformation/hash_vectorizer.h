/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_HASH_VECTORIZER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_HASH_VECTORIZER_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace brave_ads::ml {

class HashVectorizer final {
 public:
  HashVectorizer();
  HashVectorizer(int bucket_count, std::vector<uint32_t> subgrams);

  HashVectorizer(const HashVectorizer&) = delete;
  HashVectorizer& operator=(const HashVectorizer&) = delete;

  ~HashVectorizer();

  std::map<uint32_t, double> GetFrequencies(const std::string& html) const;

  std::vector<uint32_t> GetSubstringSizes() const;

  int GetBucketCount() const;

 private:
  std::vector<uint32_t> substring_sizes_;
  int bucket_count_;
};

}  // namespace brave_ads::ml

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_TRANSFORMATION_HASH_VECTORIZER_H_
