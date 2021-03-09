/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/transformation/hash_vectorizer.h"
#include "third_party/zlib/zlib.h"

namespace ads {
namespace ml {

namespace {
const int kMaximumHtmlLengthToClassify = (1 << 20);
const int kMaximumSubLen = 6;
const int kDefaultBucketCount = 10000;
}  // namespace

HashVectorizer::HashVectorizer() {
  for (int i = 1; i <= kMaximumSubLen; ++i) {
    substring_sizes_.push_back(i);
  }
  bucket_count_ = kDefaultBucketCount;
}

HashVectorizer::~HashVectorizer() = default;

HashVectorizer::HashVectorizer(const int bucket_count,
                               const std::vector<int>& subgrams) {
  for (size_t i = 0; i < subgrams.size(); ++i) {
    substring_sizes_.push_back(subgrams[i]);
  }
  bucket_count_ = bucket_count;
}

HashVectorizer::HashVectorizer(const HashVectorizer& hash_vectorizer) {
  bucket_count_ = hash_vectorizer.GetBucketCount();
  substring_sizes_ = hash_vectorizer.GetSubstringSizes();
}

std::vector<uint32_t> HashVectorizer::GetSubstringSizes() const {
  return substring_sizes_;
}

int HashVectorizer::GetBucketCount() const {
  return bucket_count_;
}

uint32_t HashVectorizer::GetHash(const std::string& substring) const {
  const char* u8str = substring.c_str();
  return crc32(crc32(0L, Z_NULL, 0), reinterpret_cast<const uint8_t*>(u8str),
               strlen(u8str));
}

std::map<uint32_t, double> HashVectorizer::GetFrequencies(
    const std::string& html) const {
  std::string data = html;
  std::map<uint32_t, double> frequencies;
  if (data.length() > kMaximumHtmlLengthToClassify) {
    data = data.substr(0, kMaximumHtmlLengthToClassify);
  }
  // get hashes of substrings for each of the substring lengths defined:
  for (const uint32_t& substring_size : substring_sizes_) {
    if (substring_size > data.length()) {
      break;
    }
    for (size_t i = 0; i < data.length() - substring_size + 1; ++i) {
      std::string ss = data.substr(i, substring_size);
      uint32_t idx = GetHash(ss);
      ++frequencies[idx % static_cast<uint32_t>(bucket_count_)];
    }
  }
  return frequencies;
}

}  // namespace ml
}  // namespace ads
