/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/prefix_util.h"

#include "base/strings/string_number_conversions.h"
#include "crypto/sha2.h"

namespace brave_rewards::internal::publisher {

std::string GetHashPrefixRaw(const std::string& publisher_key,
                             size_t prefix_size) {
  DCHECK(!publisher_key.empty());
  DCHECK(prefix_size > 0 && prefix_size <= kMaxPrefixSize);
  std::string hash = crypto::SHA256HashString(publisher_key);
  hash.resize(prefix_size);
  return hash;
}

std::string GetHashPrefixInHex(const std::string& publisher_key,
                               size_t prefix_size) {
  const std::string raw = GetHashPrefixRaw(publisher_key, prefix_size);
  return base::HexEncode(raw.data(), raw.size());
}

}  // namespace brave_rewards::internal::publisher
