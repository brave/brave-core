/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BECH32_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BECH32_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"

namespace brave_wallet::bech32 {

enum class Encoding {
  kBech32,
  kBech32m,
};

struct DecodeResult {
  DecodeResult();
  ~DecodeResult();
  DecodeResult(const DecodeResult&);
  DecodeResult& operator=(const DecodeResult&);
  DecodeResult(DecodeResult&&);
  DecodeResult& operator=(DecodeResult&&);

  Encoding encoding;
  std::string hrp;
  std::vector<uint8_t> data;
  uint8_t witness = 0;
};

// Bech32/Bech32m encode.
std::string Encode(base::span<const uint8_t> payload,
                   std::string_view hrp,
                   Encoding encoding);

// Bech32/Bech32m encode for bitcoin. Adds 5-bit witness version before payload.
std::string EncodeForBitcoin(base::span<const uint8_t> payload,
                             std::string_view hrp,
                             uint8_t witness_version);

// Bech32/Bech32m decode.
std::optional<DecodeResult> Decode(std::string_view payload);

// Bech32/Bech32m decode for bitcoin. Expects 5-bit witness version before
// payload.
std::optional<DecodeResult> DecodeForBitcoin(std::string_view payload);

}  // namespace brave_wallet::bech32

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BECH32_H_
