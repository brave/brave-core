/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_HTTP_PARTITIONED_HOST_STATE_MAP_H_
#define BRAVE_NET_HTTP_PARTITIONED_HOST_STATE_MAP_H_

#include <array>
#include <optional>
#include <string>

#include "base/auto_reset.h"
#include "base/containers/span.h"
#include "base/ranges/algorithm.h"
#include "crypto/sha2.h"
#include "net/base/net_export.h"

namespace net {

// Implements partitioning support for structures in TransportSecurityState.
class NET_EXPORT PartitionedHostStateMapBase {
 public:
  using HashedHost = std::array<uint8_t, crypto::kSHA256Length>;

  PartitionedHostStateMapBase();
  ~PartitionedHostStateMapBase();

  PartitionedHostStateMapBase(const PartitionedHostStateMapBase&) = delete;
  PartitionedHostStateMapBase& operator=(const PartitionedHostStateMapBase&) =
      delete;

  // Stores scoped partition hash for use in subsequent calls.
  base::AutoReset<std::optional<HashedHost>> SetScopedPartitionHash(
      std::optional<HashedHost> partition_hash);
  // Returns true if |partition_hash_| is set. The value may be empty.
  bool HasPartitionHash() const;
  // Returns true if |partition_hash_| contains a non empty valid hash.
  bool IsPartitionHashValid() const;
  // Creates a host hash by concatenating first 16 bytes (half of SHA256) from
  // |k| and first 16 bytes from |partition_hash_|.
  // CHECKs if |partition_hash_| is not valid.
  HashedHost GetKeyWithPartitionHash(const HashedHost& k) const;

 private:
  // Partition hash can be of these values:
  //   nullopt - unpartitioned;
  //   empty array - invalid/opaque partition, i.e. shouldn't be stored;
  //   non-empty array - valid partition.
  // Where empty is defined as zero-initialized.
  std::optional<HashedHost> partition_hash_;
};

// Allows data partitioning using half key from PartitionHash. The class mimics
// std::map interface just enough to replace unpartitioned maps in
// TransportSecurityState.
template <typename T>
class NET_EXPORT PartitionedHostStateMap : public PartitionedHostStateMapBase {
 public:
  using iterator = typename T::iterator;
  using const_iterator = typename T::const_iterator;
  using key_type = typename T::key_type;
  using mapped_type = typename T::mapped_type;
  using value_type = typename T::value_type;
  using size_type = typename T::size_type;

  const_iterator begin() const noexcept { return map_.begin(); }
  const_iterator end() const noexcept { return map_.end(); }

  size_type size() const noexcept { return map_.size(); }
  iterator erase(const_iterator position) { return map_.erase(position); }
  void clear() noexcept { map_.clear(); }

  size_type erase(const key_type& k) {
    if (!HasPartitionHash()) {
      return map_.erase(k);
    }

    if (!IsPartitionHashValid()) {
      return size_type();
    }

    return map_.erase(GetKeyWithPartitionHash(k));
  }

  mapped_type& operator[](const key_type& k) {
    if (!HasPartitionHash()) {
      return map_[k];
    }

    if (!IsPartitionHashValid()) {
      // Return a temporary item reference when the partition is invalid.
      // The item in this case shouldn't be used or persisted.
      temporary_item_ = mapped_type();
      return temporary_item_;
    }

    return map_[GetKeyWithPartitionHash(k)];
  }

  const_iterator find(const key_type& k) const {
    if (!HasPartitionHash()) {
      return map_.find(k);
    }

    if (!IsPartitionHashValid()) {
      return map_.end();
    }

    return map_.find(GetKeyWithPartitionHash(k));
  }

  // Removes all items with similar first 16 bytes of |k|, effectively ignoring
  // partition hash part.
  bool DeleteDataInAllPartitions(const key_type& k) {
    static_assert(crypto::kSHA256Length == sizeof(key_type));
    auto equal_range_pair = base::ranges::equal_range(
        map_, base::span(k).template first<crypto::kSHA256Length / 2>(),
        [](const auto& v1, const auto& v2) {
          // Mimic std::less by calling memcmp on base::span arrays.
          DCHECK(v1.size() == v2.size());
          return memcmp(v1.data(), v2.data(), v1.size()) < 0;
        },
        [](const value_type& v) {
          return base::span(v.first)
              .template first<crypto::kSHA256Length / 2>();
        });

    if (equal_range_pair.first == equal_range_pair.second) {
      return false;
    }

    map_.erase(equal_range_pair.first, equal_range_pair.second);
    return true;
  }

 private:
  T map_;
  mapped_type temporary_item_;
};

}  // namespace net

#endif  // BRAVE_NET_HTTP_PARTITIONED_HOST_STATE_MAP_H_
