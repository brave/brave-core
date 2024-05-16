/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/rsa.h"

#include <vector>

#include "base/base64.h"
#include "third_party/boringssl/src/include/openssl/bn.h"
#include "third_party/boringssl/src/include/openssl/bytestring.h"
#include "third_party/boringssl/src/include/openssl/mem.h"
#include "third_party/boringssl/src/include/openssl/rsa.h"

namespace web_discovery {

namespace {

constexpr size_t kRsaKeySize = 2048;

}  // namespace

RSAKeyInfo::RSAKeyInfo() = default;
RSAKeyInfo::~RSAKeyInfo() = default;

std::unique_ptr<RSAKeyInfo> GenerateRSAKeyPair() {
  bssl::UniquePtr<EVP_PKEY_CTX> ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
  BIGNUM* bn = BN_new();
  if (!ctx || !bn || !BN_set_word(bn, RSA_F4)) {
    return nullptr;
  }

  if (!EVP_PKEY_keygen_init(ctx.get()) ||
      !EVP_PKEY_CTX_set_rsa_keygen_pubexp(ctx.get(), bn) ||
      !EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), kRsaKeySize)) {
    return nullptr;
  }

  auto info = std::make_unique<RSAKeyInfo>();

  EVP_PKEY* private_key = nullptr;
  if (!EVP_PKEY_keygen(ctx.get(), &private_key)) {
    return nullptr;
  }

  info->key_pair = bssl::UniquePtr<EVP_PKEY>(private_key);

  CBB pub_cbb;
  uint8_t* pub_der;
  size_t pub_der_len;
  if (!CBB_init(&pub_cbb, 0) ||
      !EVP_marshal_public_key(&pub_cbb, private_key) ||
      !CBB_finish(&pub_cbb, &pub_der, &pub_der_len)) {
    CBB_cleanup(&pub_cbb);
    return nullptr;
  }

  info->public_key_b64 = base::Base64Encode(
      base::span<uint8_t>(static_cast<uint8_t*>(pub_der), pub_der_len));
  OPENSSL_free(pub_der);

  CBB priv_cbb;
  uint8_t* priv_der;
  size_t priv_der_len;
  if (!CBB_init(&priv_cbb, 0) ||
      !EVP_marshal_private_key(&priv_cbb, private_key) ||
      !CBB_finish(&priv_cbb, &priv_der, &priv_der_len)) {
    CBB_cleanup(&priv_cbb);
    return nullptr;
  }

  info->private_key_b64 = base::Base64Encode(
      base::span<uint8_t>(static_cast<uint8_t*>(priv_der), priv_der_len));
  OPENSSL_free(priv_der);

  return info;
}

EVPKeyPtr ImportRSAKeyPair(const std::string& private_key_b64) {
  std::string decoded_key;
  if (!base::Base64Decode(private_key_b64, &decoded_key)) {
    return nullptr;
  }

  const unsigned char* pkey_data =
      reinterpret_cast<const unsigned char*>(decoded_key.data());
  return EVPKeyPtr(
      d2i_PrivateKey(EVP_PKEY_RSA, nullptr, &pkey_data, decoded_key.length()));
}

std::optional<std::string> RSASign(const EVPKeyPtr& key,
                                   base::span<uint8_t> message) {
  CHECK(key);
  bssl::ScopedEVP_MD_CTX ctx;

  if (!EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr,
                          key.get())) {
    return std::nullopt;
  }

  size_t sig_len;
  // Write max size of signature to sig_len
  if (!EVP_DigestSign(ctx.get(), nullptr, &sig_len, message.data(),
                      message.size())) {
    return std::nullopt;
  }

  std::vector<uint8_t> sig(sig_len);
  // Write actual signature to sig
  if (!EVP_DigestSign(ctx.get(), sig.data(), &sig_len, message.data(),
                      message.size())) {
    return std::nullopt;
  }

  // Truncate signature to actual sig_len
  sig.resize(sig_len);
  return base::Base64Encode(sig);
}

}  // namespace web_discovery
