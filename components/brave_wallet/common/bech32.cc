/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/bech32.h"

#include <vector>

#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/to_vector.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_rust.h"
#include "brave/components/brave_wallet/rust/lib.rs.h"
#include "brave/third_party/bitcoin-core/src/src/bech32.h"
#include "brave/third_party/bitcoin-core/src/src/util/strencodings.h"

namespace brave_wallet::bech32 {

namespace {

std::string EncodeInternal(Encoding encoding,
                           base::span<const uint8_t> payload,
                           std::string_view hrp,
                           std::optional<uint8_t> witness_version) {
  std::vector<uint8_t> input;
  input.reserve(payload.size() * 8 / 5 + (witness_version ? 1 : 0));
  if (witness_version) {
    input.push_back(*witness_version);
  }
  ConvertBits<8, 5, true>([&](unsigned char c) { input.push_back(c); },
                          payload.begin(), payload.end());

  return ::bech32::Encode(encoding == Encoding::kBech32
                              ? ::bech32::Encoding::BECH32
                              : ::bech32::Encoding::BECH32M,
                          std::string(hrp), input);
}

}  // namespace

DecodeResult::DecodeResult() = default;
DecodeResult::~DecodeResult() = default;
DecodeResult::DecodeResult(const DecodeResult&) = default;
DecodeResult& DecodeResult::operator=(const DecodeResult&) = default;
DecodeResult::DecodeResult(DecodeResult&&) = default;
DecodeResult& DecodeResult::operator=(DecodeResult&&) = default;

std::string Encode(base::span<const uint8_t> payload,
                   std::string_view hrp,
                   Encoding encoding) {
  return EncodeInternal(encoding, payload, hrp, std::nullopt);
}

std::string EncodeForBitcoin(base::span<const uint8_t> payload,
                             std::string_view hrp,
                             uint8_t witness_version) {
  DCHECK_LE(witness_version, 16);
  return EncodeInternal(
      witness_version == 0 ? Encoding::kBech32 : Encoding::kBech32m, payload,
      hrp, witness_version);
}

std::optional<DecodeResult> DecodeForBitcoin(std::string_view payload) {
  auto bech_result = ::bech32::Decode(std::string(payload));

  if (bech_result.encoding != ::bech32::Encoding::BECH32 &&
      bech_result.encoding != ::bech32::Encoding::BECH32M) {
    return std::nullopt;
  }

  DecodeResult result;
  result.encoding = bech_result.encoding == ::bech32::Encoding::BECH32
                        ? Encoding::kBech32
                        : Encoding::kBech32m;
  auto base32_span = base::span(bech_result.data);
  if (base32_span.empty()) {
    return std::nullopt;
  }
  result.witness = base32_span.front();
  base32_span = base32_span.subspan(1u);
  result.data.reserve((base32_span.size() * 5) / 8);
  if (!ConvertBits<5, 8, false>(
          [&](unsigned char c) { result.data.push_back(c); },
          base32_span.begin(), base32_span.end())) {
    return std::nullopt;
  }

  result.hrp = bech_result.hrp;

  return result;
}

std::optional<DecodeResult> Decode(std::string_view payload) {
  if (!base::IsStringASCII(payload)) {
    return std::nullopt;
  }

  auto bech_result = decode_bech32(std::string(payload));
  if (!bech_result->is_ok()) {
    return std::nullopt;
  }

  DecodeResult result;

  auto& unwrapped = bech_result->unwrap();

  if (unwrapped.variant() == Bech32DecodeVariant::Bech32) {
    result.encoding = Encoding::kBech32;
  } else if (unwrapped.variant() == Bech32DecodeVariant::Bech32m) {
    result.encoding = Encoding::kBech32m;
  } else {
    return std::nullopt;
  }

  result.hrp = std::string(unwrapped.hrp());
  result.data = base::ToVector(unwrapped.data());

  return result;
}

}  // namespace brave_wallet::bech32
