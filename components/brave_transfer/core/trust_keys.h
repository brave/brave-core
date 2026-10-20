// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_TRUST_KEYS_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_TRUST_KEYS_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/time/time.h"

namespace brave_transfer {

inline constexpr size_t kKeySize = 32;
inline constexpr size_t kTagSize = 8;
inline constexpr int64_t kEpochSeconds = 900;

using Key = std::array<uint8_t, kKeySize>;
using Tag = std::array<uint8_t, kTagSize>;

// DNS-SD service type the responder registers (PROTOCOL.md §3.3).
inline constexpr char kServiceType[] = "_brave-xfer._tcp";

// Discovery epoch for `time`; times before the Unix epoch map to 0.
uint64_t EpochForTime(base::Time time);

// Keys every device in one Brave Sync chain can derive (PROTOCOL.md §3).
// `seed` is brave_sync::crypto::PassphraseToBytes32() of the chain's 24-word
// sync code; converting the code is left to the embedder.
class ChainKeys {
 public:
  static std::optional<ChainKeys> FromSeed(base::span<const uint8_t> seed);
  static std::optional<ChainKeys> FromRootKey(
      base::span<const uint8_t> root_key);

  ChainKeys(const ChainKeys&);
  ChainKeys& operator=(const ChainKeys&);
  ~ChainKeys();

  const Key& root_key() const { return root_key_; }
  const Key& psk() const { return psk_; }
  const Key& discovery_key() const { return discovery_key_; }

  Tag TagForEpoch(uint64_t epoch) const;
  // Tags for epochs t-1, t and t+1, skipping negative epochs.
  std::vector<Tag> AcceptedTags(base::Time now) const;
  bool AcceptsTag(base::span<const uint8_t> tag, base::Time now) const;

  // TXT record entries for the current epoch: {"v": "1", "c": <hex tag>}.
  base::flat_map<std::string, std::string> TxtRecord(base::Time now) const;
  // Whether a browsed instance advertises v=1 and one of AcceptedTags().
  // Returns the matching tag.
  std::optional<Tag> MatchTxtRecord(
      const base::flat_map<std::string, std::string>& record,
      base::Time now) const;

  bool operator==(const ChainKeys& other) const;

 private:
  explicit ChainKeys(const Key& root_key);

  Key root_key_;
  Key psk_;
  Key discovery_key_;
};

// Long-term X25519 identity keys (PROTOCOL.md §3.4).
namespace static_key {

Key GeneratePrivateKey();
Key PublicKey(const Key& private_key);
// First 16 bytes of SHA-256(public key) as lowercase hex in groups of four,
// separated by spaces.
std::string Fingerprint(base::span<const uint8_t> public_key);

}  // namespace static_key

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_TRUST_KEYS_H_
