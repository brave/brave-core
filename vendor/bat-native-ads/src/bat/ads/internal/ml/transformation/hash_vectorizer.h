/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_TRANSFORMATION_HASH_VECTORIZER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_TRANSFORMATION_HASH_VECTORIZER_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ads::ml {

class HashVectorizer final {
 public:
  HashVectorizer();
  HashVectorizer(int bucket_count, const std::vector<int>& subgrams);

  HashVectorizer(const HashVectorizer& other) = delete;
  HashVectorizer& operator=(const HashVectorizer& other) = delete;

  HashVectorizer(HashVectorizer&& other) noexcept = delete;
  HashVectorizer& operator=(HashVectorizer&& other) noexcept = delete;

  ~HashVectorizer();

  std::map<uint32_t, double> GetFrequencies(const std::string& html) const;

  std::vector<uint32_t> GetSubstringSizes() const;

  int GetBucketCount() const;

 private:
  std::vector<uint32_t> substring_sizes_;
  int bucket_count_;
};

}  // namespace ads::ml

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_TRANSFORMATION_HASH_VECTORIZER_H_
