/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/wallet_connect/encryptor.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "crypto/encryptor.h"
#include "crypto/hmac.h"
#include "crypto/random.h"
#include "crypto/symmetric_key.h"
#include "third_party/boringssl/src/include/openssl/mem.h"

using crypto::SymmetricKey;

namespace wallet_connect {

namespace {
const size_t kAESBlockSize = 16;
const size_t kHMACDigestLength = 32;
}  // namespace

Encryptor::Encryptor(const std::array<uint8_t, 32>& key) : key_(key) {}
Encryptor::~Encryptor() {
  // zero out key_
  OPENSSL_cleanse(key_.data(), key_.size());
}

base::expected<types::EncryptionPayload, std::string> Encryptor::Encrypt(
    const std::vector<uint8_t>& data) {
  std::vector<uint8_t> iv(kAESBlockSize);
  crypto::RandBytes(iv);

  auto symmetric_key = SymmetricKey::Import(
      SymmetricKey::Algorithm::AES, std::string(key_.begin(), key_.end()));
  CHECK(symmetric_key);
  crypto::Encryptor encryptor;
  std::vector<uint8_t> output;
  if (!encryptor.Init(symmetric_key.get(), crypto::Encryptor::Mode::CBC, iv) ||
      !encryptor.Encrypt(data, &output)) {
    return base::unexpected("AES-256-CBC encrypt failed");
  }

  std::vector<uint8_t> data_to_sign(output.begin(), output.end());
  data_to_sign.insert(data_to_sign.end(), iv.begin(), iv.end());

  std::vector<uint8_t> expected_signature(kHMACDigestLength);
  crypto::HMAC hmac(crypto::HMAC::HashAlgorithm::SHA256);
  if (!hmac.Init(key_) || !hmac.Sign(data_to_sign, expected_signature)) {
    return base::unexpected("Calculate HMAC failed");
  }

  types::EncryptionPayload payload;
  payload.iv = base::ToLowerASCII(base::HexEncode(iv));
  payload.data = base::ToLowerASCII(base::HexEncode(output));
  payload.hmac = base::ToLowerASCII(base::HexEncode(expected_signature));

  return payload;
}

base::expected<std::vector<uint8_t>, std::string> Encryptor::Decrypt(
    const types::EncryptionPayload& payload) {
  std::vector<uint8_t> ciphertext;
  std::vector<uint8_t> hmac_bytes;
  std::vector<uint8_t> iv;
  if (!base::HexStringToBytes(payload.data, &ciphertext) ||
      !base::HexStringToBytes(payload.hmac, &hmac_bytes) ||
      !base::HexStringToBytes(payload.iv, &iv)) {
    return base::unexpected("Payload contains invalid hex string: " +
                            payload.ToValue().DebugString());
  }

  std::vector<uint8_t> data_to_sign(ciphertext.begin(), ciphertext.end());
  data_to_sign.insert(data_to_sign.end(), iv.begin(), iv.end());

  crypto::HMAC hmac(crypto::HMAC::HashAlgorithm::SHA256);
  if (!hmac.Init(key_) || !hmac.Verify(data_to_sign, hmac_bytes)) {
    return base::unexpected("HMAC mismatched");
  }

  auto symmetric_key = SymmetricKey::Import(
      SymmetricKey::Algorithm::AES, std::string(key_.begin(), key_.end()));
  CHECK(symmetric_key);
  crypto::Encryptor encryptor;
  std::vector<uint8_t> output;
  if (!encryptor.Init(symmetric_key.get(), crypto::Encryptor::Mode::CBC, iv) ||
      !encryptor.Decrypt(ciphertext, &output)) {
    return base::unexpected("AES-256-CBC decrypt failed");
  }
  return output;
}

}  // namespace wallet_connect
