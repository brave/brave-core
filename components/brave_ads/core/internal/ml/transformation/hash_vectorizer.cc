/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/hash_vectorizer.h"

#include <cstdint>
#include <utility>

#include "third_party/zlib/zlib.h"

namespace brave_ads::ml {

namespace {

constexpr int kMaximumHtmlLengthToClassify = 1 << 20;
constexpr int kMaximumSubLen = 6;
constexpr int kDefaultBucketCount = 10'000;

uint32_t GetHash(const std::string& text) {
  const char* const u8str = text.c_str();
  return crc32(crc32(0L, Z_NULL, 0), reinterpret_cast<const uint8_t*>(u8str),
               strlen(u8str));
}

}  // namespace

HashVectorizer::HashVectorizer() {
  for (int i = 1; i <= kMaximumSubLen; ++i) {
    substring_sizes_.push_back(i);
  }
  bucket_count_ = kDefaultBucketCount;
}

HashVectorizer::~HashVectorizer() = default;

HashVectorizer::HashVectorizer(int bucket_count, std::vector<uint32_t> subgrams)
    : substring_sizes_(std::move(subgrams)), bucket_count_(bucket_count) {}

std::vector<uint32_t> HashVectorizer::GetSubstringSizes() const {
  return substring_sizes_;
}

int HashVectorizer::GetBucketCount() const {
  return bucket_count_;
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
      const std::string ss = data.substr(i, substring_size);
      const uint32_t idx = GetHash(ss);
      ++frequencies[idx % static_cast<uint32_t>(bucket_count_)];
    }
  }
  return frequencies;
}

}  // namespace brave_ads::ml
