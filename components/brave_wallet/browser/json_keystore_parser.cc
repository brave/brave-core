/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_keystore_parser.h"

#include "base/containers/extend.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "crypto/encryptor.h"
#include "crypto/kdf.h"
#include "crypto/symmetric_key.h"

namespace brave_wallet {

inline constexpr size_t kPrivateKeySize = 32;
inline constexpr size_t kDerivedKeySize = 32;

namespace {

bool UTCPasswordVerification(
    base::span<const uint8_t, kDerivedKeySize> derived_key,
    base::span<const uint8_t> ciphertext,
    base::span<const uint8_t> mac) {
  std::vector<uint8_t> mac_verification_input;
  mac_verification_input.reserve(kDerivedKeySize / 2 + ciphertext.size());

  base::Extend(mac_verification_input, derived_key.last(kDerivedKeySize / 2));
  base::Extend(mac_verification_input, ciphertext);

  return KeccakHash(mac_verification_input) == mac;
}

std::optional<std::vector<uint8_t>> UTCDecryptPrivateKey(
    base::span<const uint8_t, kDerivedKeySize> derived_key,
    base::span<const uint8_t> ciphertext,
    base::span<const uint8_t> iv) {
  crypto::SymmetricKey decryption_key(derived_key.first(kDerivedKeySize / 2));
  crypto::Encryptor encryptor;
  if (!encryptor.Init(&decryption_key, crypto::Encryptor::Mode::CTR,
                      std::vector<uint8_t>())) {
    return std::nullopt;
  }
  if (!encryptor.SetCounter(iv)) {
    return std::nullopt;
  }

  std::vector<uint8_t> private_key;
  if (!encryptor.Decrypt(ciphertext, &private_key)) {
    return std::nullopt;
  }

  return private_key;
}

}  // namespace

std::optional<std::vector<uint8_t>> DecryptPrivateKeyFromJsonKeystore(
    const std::string& password,
    const base::Value::Dict& dict) {
  if (password.empty()) {
    return std::nullopt;
  }

  // check version
  auto version = dict.FindInt("version");
  if (!version || *version != 3) {
    return std::nullopt;
  }

  const auto* crypto = dict.FindDict("crypto");
  if (!crypto) {
    return std::nullopt;
  }
  const auto* kdf = crypto->FindString("kdf");
  if (!kdf) {
    return std::nullopt;
  }
  const auto* kdfparams = crypto->FindDict("kdfparams");
  if (!kdfparams) {
    return std::nullopt;
  }
  auto dklen = kdfparams->FindInt("dklen");
  if (!dklen) {
    return std::nullopt;
  }
  // TODO(apaymyshev): web3.js parser allows more bytes for `dklen`, but uses
  // only first 32
  // https://github.com/web3/web3.js/blob/4.x/packages/web3-eth-accounts/src/account.ts#L857-L868
  if (*dklen != 32) {
    return std::nullopt;
  }
  const auto* salt = kdfparams->FindString("salt");
  if (!salt) {
    return std::nullopt;
  }
  std::vector<uint8_t> salt_bytes;
  if (!base::HexStringToBytes(*salt, &salt_bytes)) {
    return std::nullopt;
  }

  std::array<uint8_t, kDerivedKeySize> derived_key = {};

  if (*kdf == "pbkdf2") {
    auto c = kdfparams->FindInt("c");
    if (!c) {
      return std::nullopt;
    }
    const auto* prf = kdfparams->FindString("prf");
    if (!prf) {
      return std::nullopt;
    }
    if (*prf != "hmac-sha256") {
      return std::nullopt;
    }

    crypto::kdf::Pbkdf2HmacSha256Params params = {
        .iterations = base::checked_cast<decltype(params.iterations)>(*c),
    };
    if (!crypto::kdf::DeriveKeyPbkdf2HmacSha256(
            params, base::as_byte_span(password),
            base::as_byte_span(salt_bytes), derived_key)) {
      return std::nullopt;
    }
  } else if (*kdf == "scrypt") {
    auto n = kdfparams->FindInt("n");
    if (!n) {
      return std::nullopt;
    }
    auto r = kdfparams->FindInt("r");
    if (!r) {
      return std::nullopt;
    }
    auto p = kdfparams->FindInt("p");
    if (!p) {
      return std::nullopt;
    }
    crypto::kdf::ScryptParams params = {
        .cost = (size_t)*n,
        .block_size = (size_t)*r,
        .parallelization = (size_t)*p,
        .max_memory_bytes = 512 * 1024 * 1024,
    };
    if (!crypto::kdf::DeriveKeyScryptNoCheck(
            params, base::as_byte_span(password),
            base::as_byte_span(salt_bytes), derived_key)) {
      return std::nullopt;
    }
  } else {
    return std::nullopt;
  }

  const auto* mac = crypto->FindString("mac");
  if (!mac) {
    return std::nullopt;
  }
  std::vector<uint8_t> mac_bytes;
  if (!base::HexStringToBytes(*mac, &mac_bytes)) {
    return std::nullopt;
  }
  const auto* ciphertext = crypto->FindString("ciphertext");
  if (!ciphertext) {
    return std::nullopt;
  }
  std::vector<uint8_t> ciphertext_bytes;
  if (!base::HexStringToBytes(*ciphertext, &ciphertext_bytes)) {
    return std::nullopt;
  }

  if (!UTCPasswordVerification(derived_key, ciphertext_bytes, mac_bytes)) {
    return std::nullopt;
  }

  const auto* cipher = crypto->FindString("cipher");
  if (!cipher) {
    return std::nullopt;
  }
  if (*cipher != "aes-128-ctr") {
    return std::nullopt;
  }

  std::vector<uint8_t> iv_bytes;
  const auto* iv = crypto->FindStringByDottedPath("cipherparams.iv");
  if (!iv) {
    return std::nullopt;
  }
  if (!base::HexStringToBytes(*iv, &iv_bytes)) {
    return std::nullopt;
  }

  auto private_key =
      UTCDecryptPrivateKey(derived_key, ciphertext_bytes, iv_bytes);
  if (!private_key || private_key->size() != kPrivateKeySize) {
    return std::nullopt;
  }

  return private_key;
}

}  // namespace brave_wallet
