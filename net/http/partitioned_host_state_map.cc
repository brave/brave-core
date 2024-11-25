/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/http/partitioned_host_state_map.h"

#include <optional>
#include <utility>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "crypto/sha2.h"
#include "net/base/network_isolation_key.h"

namespace {

bool IsEmptyPartitionHash(
    const net::PartitionedHostStateMapBase::HashedHost& hashed_host) {
  return base::ranges::all_of(hashed_host.begin(), hashed_host.end(),
                              [](uint8_t i) { return i == 0; });
}

}  // namespace

namespace net {

PartitionedHostStateMapBase::PartitionedHostStateMapBase() = default;
PartitionedHostStateMapBase::~PartitionedHostStateMapBase() = default;

base::AutoReset<std::optional<PartitionedHostStateMapBase::HashedHost>>
PartitionedHostStateMapBase::SetScopedPartitionHash(
    std::optional<HashedHost> partition_hash) {
  return base::AutoReset<std::optional<HashedHost>>(&partition_hash_,
                                                    std::move(partition_hash));
}

bool PartitionedHostStateMapBase::HasPartitionHash() const {
  return partition_hash_.has_value();
}

bool PartitionedHostStateMapBase::IsPartitionHashValid() const {
  return partition_hash_ && !IsEmptyPartitionHash(*partition_hash_);
}

PartitionedHostStateMapBase::HashedHost
PartitionedHostStateMapBase::GetKeyWithPartitionHash(
    const HashedHost& k) const {
  CHECK(IsPartitionHashValid());
  if (k == *partition_hash_) {
    return k;
  }

  HashedHost result;
  static_assert(result.size() == crypto::kSHA256Length,
                "Unexpected HashedHost size");
  base::span(result).first<crypto::kSHA256Length / 2>().copy_from(
      base::span(k).first<crypto::kSHA256Length / 2>());
  base::span(result).last<crypto::kSHA256Length / 2>().copy_from(
      base::span(*partition_hash_).first<crypto::kSHA256Length / 2>());
  return result;
}

}  // namespace net
