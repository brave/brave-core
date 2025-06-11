/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ENCODING_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ENCODING_UTILS_H_

#include <array>
#include <string>
#include <vector>

#include "base/containers/span.h"

namespace brave_wallet {

// Size of Ed25519\Sr25519 public keys.
inline constexpr size_t kSs58PublicKeySize = 32u;

// Encodes Ed25519 pr Sr25519 public key adding
// special prefix and checksum.
struct Ss58Address {
  Ss58Address();
  ~Ss58Address();
  Ss58Address(Ss58Address& addr) = delete;
  Ss58Address& operator=(const Ss58Address& addr) = delete;
  Ss58Address(Ss58Address&& addr);
  Ss58Address& operator=(Ss58Address&& addr);
  uint16_t prefix = 0;
  // ed25519 or sr25519 public key.
  std::array<uint8_t, kSs58PublicKeySize> public_key = {};

  std::optional<std::string> Encode();
  static std::optional<Ss58Address> Decode(const std::string& str);
};

// A bridge function to call DecodeBase58 in bitcoin-core.
// It will return false if length of decoded byte array does not match len
// param.
bool Base58Decode(const std::string& str,
                  std::vector<uint8_t>* ret,
                  int len,
                  bool strict = true);
std::optional<std::vector<uint8_t>> Base58Decode(const std::string& str,
                                                 int len,
                                                 bool strict = true);
// A bridge function to call EncodeBase58 in bitcoin-core.
std::string Base58Encode(base::span<const uint8_t> bytes);
std::string Base58EncodeWithCheck(const std::vector<uint8_t>& bytes);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ENCODING_UTILS_H_
