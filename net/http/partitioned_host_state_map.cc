/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/http/partitioned_host_state_map.h"

#include <utility>

#include "base/strings/strcat.h"
#include "crypto/sha2.h"
#include "net/base/network_isolation_key.h"

namespace net {

PartitionedHostStateMapBase::PartitionedHostStateMapBase() = default;
PartitionedHostStateMapBase::~PartitionedHostStateMapBase() = default;

base::AutoReset<absl::optional<std::string>>
PartitionedHostStateMapBase::SetScopedPartitionHash(
    absl::optional<std::string> partition_hash) {
  CHECK(!partition_hash || partition_hash->empty() ||
        partition_hash->size() == crypto::kSHA256Length);
  return base::AutoReset<absl::optional<std::string>>(
      &partition_hash_, std::move(partition_hash));
}

bool PartitionedHostStateMapBase::HasPartitionHash() const {
  return partition_hash_.has_value();
}

bool PartitionedHostStateMapBase::IsPartitionHashValid() const {
  return partition_hash_ && !partition_hash_->empty();
}

std::string PartitionedHostStateMapBase::GetKeyWithPartitionHash(
    const std::string& k) const {
  CHECK(IsPartitionHashValid());
  if (k == *partition_hash_) {
    return k;
  }
  return base::StrCat({GetHalfKey(k), GetHalfKey(*partition_hash_)});
}

// static
base::StringPiece PartitionedHostStateMapBase::GetHalfKey(base::StringPiece k) {
  CHECK_EQ(k.size(), crypto::kSHA256Length);
  const size_t kHalfSHA256HashLength = crypto::kSHA256Length / 2;
  return k.substr(0, kHalfSHA256HashLength);
}

}  // namespace net
