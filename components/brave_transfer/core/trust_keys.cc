// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/trust_keys.h"

#include <string_view>

#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "crypto/hash.h"
#include "crypto/hmac.h"
#include "crypto/kdf.h"
#include "crypto/random.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_transfer {

namespace {

constexpr std::string_view kRootSalt = "brave-transfer";
constexpr std::string_view kRootInfo = "v1/root";
constexpr std::string_view kPskInfo = "v1/chain-psk";
constexpr std::string_view kDiscoveryInfo = "v1/discovery";
constexpr size_t kFingerprintBytes = 16;

Key Hkdf(base::span<const uint8_t> ikm,
         std::string_view salt,
         std::string_view info) {
  return crypto::kdf::Hkdf<kKeySize>(crypto::hash::kSha256, ikm,
                                     base::as_byte_span(salt),
                                     base::as_byte_span(info));
}

}  // namespace

uint64_t EpochForTime(base::Time time) {
  const double seconds = time.InSecondsFSinceUnixEpoch();
  if (seconds <= 0) {
    return 0;
  }
  return static_cast<uint64_t>(seconds) / kEpochSeconds;
}

// static
std::optional<ChainKeys> ChainKeys::FromSeed(base::span<const uint8_t> seed) {
  if (seed.size() != kKeySize) {
    return std::nullopt;
  }
  return ChainKeys(Hkdf(seed, kRootSalt, kRootInfo));
}

// static
std::optional<ChainKeys> ChainKeys::FromRootKey(
    base::span<const uint8_t> root_key) {
  if (root_key.size() != kKeySize) {
    return std::nullopt;
  }
  Key key;
  base::span(key).copy_from(root_key);
  return ChainKeys(key);
}

ChainKeys::ChainKeys(const Key& root_key)
    : root_key_(root_key),
      psk_(Hkdf(root_key, "", kPskInfo)),
      discovery_key_(Hkdf(root_key, "", kDiscoveryInfo)) {}

ChainKeys::ChainKeys(const ChainKeys&) = default;
ChainKeys& ChainKeys::operator=(const ChainKeys&) = default;
ChainKeys::~ChainKeys() = default;

bool ChainKeys::operator==(const ChainKeys& other) const = default;

Tag ChainKeys::TagForEpoch(uint64_t epoch) const {
  const auto mac = crypto::hmac::SignSha256(discovery_key_,
                                            base::U64ToBigEndian(epoch));
  Tag tag;
  base::span(tag).copy_from(base::span(mac).first<kTagSize>());
  return tag;
}

std::vector<Tag> ChainKeys::AcceptedTags(base::Time now) const {
  const uint64_t epoch = EpochForTime(now);
  std::vector<Tag> tags;
  if (epoch > 0) {
    tags.push_back(TagForEpoch(epoch - 1));
  }
  tags.push_back(TagForEpoch(epoch));
  tags.push_back(TagForEpoch(epoch + 1));
  return tags;
}

bool ChainKeys::AcceptsTag(base::span<const uint8_t> tag,
                           base::Time now) const {
  if (tag.size() != kTagSize) {
    return false;
  }
  for (const Tag& accepted : AcceptedTags(now)) {
    if (base::span(accepted) == tag) {
      return true;
    }
  }
  return false;
}

base::flat_map<std::string, std::string> ChainKeys::TxtRecord(
    base::Time now) const {
  return {{"v", "1"},
          {"c", base::ToLowerASCII(
                    base::HexEncode(TagForEpoch(EpochForTime(now))))}};
}

std::optional<Tag> ChainKeys::MatchTxtRecord(
    const base::flat_map<std::string, std::string>& record,
    base::Time now) const {
  const auto version = record.find("v");
  const auto tag_hex = record.find("c");
  if (version == record.end() || version->second != "1" ||
      tag_hex == record.end()) {
    return std::nullopt;
  }
  Tag tag;
  if (!base::HexStringToSpan(tag_hex->second, tag)) {
    return std::nullopt;
  }
  if (!AcceptsTag(tag, now)) {
    return std::nullopt;
  }
  return tag;
}

namespace static_key {

Key GeneratePrivateKey() {
  return crypto::RandBytesAsArray<kKeySize>();
}

Key PublicKey(const Key& private_key) {
  Key public_key;
  X25519_public_from_private(public_key.data(), private_key.data());
  return public_key;
}

std::string Fingerprint(base::span<const uint8_t> public_key) {
  const auto digest = crypto::hash::Sha256(public_key);
  const std::string hex = base::ToLowerASCII(
      base::HexEncode(base::span(digest).first<kFingerprintBytes>()));
  std::string out;
  for (size_t i = 0; i < hex.size(); i += 4) {
    if (!out.empty()) {
      out += ' ';
    }
    out += hex.substr(i, 4);
  }
  return out;
}

}  // namespace static_key

}  // namespace brave_transfer
