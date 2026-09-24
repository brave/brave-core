/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bip39.h"

#include "base/containers/span.h"
#include "base/strings/strcat.h"
#include "brave/third_party/bip39wally-core-native/include/wally_bip39.h"
#include "crypto/kdf.h"
#include "crypto/process_bound_string.h"

namespace brave_wallet::bip39 {

namespace {

// https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki#generating-the-mnemonic
inline constexpr uint32_t kMaxSupportedEntropySize = 32;

// https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki#from-mnemonic-to-seed
inline constexpr uint32_t kPBKDF2Iterations = 2048;

std::optional<std::string> GenerateMnemonicInternal(
    base::span<const uint8_t> entropy) {
  char* words = nullptr;
  if (bip39_mnemonic_from_bytes(nullptr, entropy.data(), entropy.size(),
                                &words) != WALLY_OK) {
    return std::nullopt;
  }
  std::string result(words);
  wally_free_string(words);
  return result;
}

bool IsValidEntropySize(size_t entropy_size) {
  // entropy size should be 128, 160, 192, 224, 256 bits
  if (entropy_size < 16 || entropy_size > 32 || entropy_size % 4 != 0) {
    return false;
  }
  return true;
}

}  // namespace

std::optional<std::string> GenerateMnemonic(base::span<const uint8_t> entropy) {
  if (!IsValidEntropySize(entropy.size())) {
    return std::nullopt;
  }
  return GenerateMnemonicInternal(entropy);
}

std::optional<std::vector<uint8_t>> MnemonicToSeed(
    std::string_view mnemonic,
    std::string_view passphrase) {
  if (!IsValidMnemonic(mnemonic)) {
    return std::nullopt;
  }

  std::vector<uint8_t> seed(kSeedSize, 0);
  const std::string salt = base::StrCat({"mnemonic", passphrase});

  if (!crypto::kdf::DeriveKeyPbkdf2HmacSha512({.iterations = kPBKDF2Iterations},
                                              base::as_byte_span(mnemonic),
                                              base::as_byte_span(salt), seed)) {
    return std::nullopt;
  }
  return seed;
}

std::optional<std::vector<uint8_t>> MnemonicToEntropy(
    std::string_view mnemonic) {
  if (!IsValidMnemonic(mnemonic)) {
    return std::nullopt;
  }

  std::vector<uint8_t> entropy(kMaxSupportedEntropySize, 0);
  size_t written = 0;
  if (bip39_mnemonic_to_bytes(nullptr, std::string(mnemonic).c_str(),
                              entropy.data(), entropy.size(),
                              &written) != WALLY_OK) {
    return std::nullopt;
  }
  entropy.resize(written);
  return entropy;
}

bool IsValidMnemonic(std::string_view mnemonic) {
  if (bip39_mnemonic_validate(nullptr, std::string(mnemonic).c_str()) !=
      WALLY_OK) {
    return false;
  }
  return true;
}

// The polkadot-sdk derives seeds from mnemonics in a way divergent from
// normal BIP-39 routines. Instead, it hashes the derived entropy instead of
// the mnemonic so we need a special routine just for Polkadot:
// https://github.com/paritytech/polkadot-sdk/blob/beb9030b249cc078b3955232074a8495e7e0302a/substrate/primitives/core/src/crypto.rs#L866-L883
// https://github.com/paritytech/polkadot-sdk/blob/beb9030b249cc078b3955232074a8495e7e0302a/substrate/utils/substrate-bip39/src/lib.rs#L52-L70
// https://wiki.polkadot.com/learn/learn-account-advanced/#portability
std::optional<std::array<uint8_t, kSeedSize>> MnemonicToEntropyToSeed(
    std::string_view mnemonic,
    std::string_view password) {
  auto entropy = bip39::MnemonicToEntropy(mnemonic);
  if (!entropy) {
    return std::nullopt;
  }

  crypto::SecureString salt;
  salt.reserve(8 + password.size());
  salt.append("mnemonic");
  salt.append(password);

  std::array<uint8_t, kSeedSize> seed = {};

  if (!crypto::kdf::DeriveKeyPbkdf2HmacSha512({.iterations = kPBKDF2Iterations},
                                              *entropy,
                                              base::as_byte_span(salt), seed)) {
    return std::nullopt;
  }

  return seed;
}

}  // namespace brave_wallet::bip39
