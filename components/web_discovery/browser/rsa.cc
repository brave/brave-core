/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/rsa.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/threading/thread_restrictions.h"
#include "crypto/rsa_private_key.h"
#include "crypto/signature_creator.h"

namespace web_discovery {

namespace {

constexpr size_t kRsaKeySize = 2048;

}  // namespace

EncodedRSAKeyPair::EncodedRSAKeyPair(std::string private_key_b64,
                                     std::string public_key_b64)
    : private_key_b64(std::move(private_key_b64)),
      public_key_b64(std::move(public_key_b64)) {}
EncodedRSAKeyPair::~EncodedRSAKeyPair() = default;

ImportedRSAKey::ImportedRSAKey() = default;
ImportedRSAKey::~ImportedRSAKey() = default;

std::unique_ptr<crypto::RSAPrivateKey> GenerateRSAKey() {
  base::AssertLongCPUWorkAllowed();
  return crypto::RSAPrivateKey::Create(kRsaKeySize);
}

std::unique_ptr<EncodedRSAKeyPair> ExportRSAKey(
    const crypto::RSAPrivateKey& private_key) {
  std::vector<uint8_t> encoded_public_key;
  std::vector<uint8_t> encoded_private_key;

  if (!private_key.ExportPrivateKey(&encoded_private_key) ||
      !private_key.ExportPublicKey(&encoded_public_key)) {
    return nullptr;
  }

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

  auto key_pair = crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(*decoded_key);
  if (!key_pair) {
    return nullptr;
  }

  std::vector<uint8_t> encoded_public_key;
  if (!key_pair->ExportPublicKey(&encoded_public_key)) {
    return nullptr;
  }

  auto info = std::make_unique<ImportedRSAKey>();

  info->private_key = std::move(key_pair);
  info->public_key_b64 = base::Base64Encode(encoded_public_key);

  return info;
}

std::optional<std::string> RSASign(crypto::RSAPrivateKey* key,
                                   base::span<uint8_t> message) {
  base::AssertLongCPUWorkAllowed();
  CHECK(key);

  std::vector<uint8_t> signature;
  auto signature_creator = crypto::SignatureCreator::Create(
      key, crypto::SignatureCreator::HashAlgorithm::SHA256);
  if (!signature_creator) {
    return std::nullopt;
  }
  if (!signature_creator->Update(message.data(), message.size())) {
    return std::nullopt;
  }
  if (!signature_creator->Final(&signature)) {
    return std::nullopt;
  }

  return base::Base64Encode(signature);
}

}  // namespace web_discovery
