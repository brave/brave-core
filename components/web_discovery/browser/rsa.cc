/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/rsa.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/threading/thread_restrictions.h"
#include "crypto/keypair.h"
#include "crypto/sign.h"

namespace web_discovery {

EncodedRSAKeyPair::EncodedRSAKeyPair(std::string private_key_b64,
                                     std::string public_key_b64)
    : private_key_b64(std::move(private_key_b64)),
      public_key_b64(std::move(public_key_b64)) {}
EncodedRSAKeyPair::~EncodedRSAKeyPair() = default;

ImportedRSAKey::ImportedRSAKey(crypto::keypair::PrivateKey private_key,
                               std::string public_key_b64)
    : private_key(std::move(private_key)),
      public_key_b64(std::move(public_key_b64)) {}
ImportedRSAKey::~ImportedRSAKey() = default;

crypto::keypair::PrivateKey GenerateRSAKey() {
  base::AssertLongCPUWorkAllowed();
  return crypto::keypair::PrivateKey::GenerateRsa2048();
}

std::unique_ptr<EncodedRSAKeyPair> ExportRSAKey(
    const crypto::keypair::PrivateKey& private_key) {
  std::vector<uint8_t> encoded_private_key = private_key.ToPrivateKeyInfo();
  std::vector<uint8_t> encoded_public_key =
      private_key.ToSubjectPublicKeyInfo();

  return std::make_unique<EncodedRSAKeyPair>(
      base::Base64Encode(encoded_private_key),
      base::Base64Encode(encoded_public_key));
}

std::unique_ptr<ImportedRSAKey> ImportRSAKey(
    const std::string& private_key_b64) {
  auto decoded_key = base::Base64Decode(private_key_b64);
  if (!decoded_key) {
    return nullptr;
  }

  auto key_pair = crypto::keypair::PrivateKey::FromPrivateKeyInfo(*decoded_key);
  if (!key_pair) {
    return nullptr;
  }

  std::vector<uint8_t> encoded_public_key = key_pair->ToSubjectPublicKeyInfo();

  return std::make_unique<ImportedRSAKey>(
      std::move(*key_pair), base::Base64Encode(encoded_public_key));
}

std::string RSASign(const crypto::keypair::PrivateKey& key,
                    base::span<uint8_t> message) {
  base::AssertLongCPUWorkAllowed();

  std::vector<uint8_t> signature_bytes = crypto::sign::Sign(
      crypto::sign::SignatureKind::RSA_PKCS1_SHA256, key, message);
  return base::Base64Encode(signature_bytes);
}

}  // namespace web_discovery
