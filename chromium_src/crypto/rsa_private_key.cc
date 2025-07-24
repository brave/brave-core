/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "crypto/rsa_private_key.h"

#include <crypto/rsa_private_key.cc>

namespace crypto {

// static
std::unique_ptr<RSAPrivateKey> RSAPrivateKey::Create(uint16_t num_bits) {
  OpenSSLErrStackTracer err_tracer(FROM_HERE);

  bssl::UniquePtr<RSA> rsa_key(RSA_new());
  bssl::UniquePtr<BIGNUM> bn(BN_new());
  if (!rsa_key.get() || !bn.get() || !BN_set_word(bn.get(), 65537L)) {
    return nullptr;
  }

  if (!RSA_generate_key_ex(rsa_key.get(), num_bits, bn.get(), nullptr)) {
    return nullptr;
  }

  std::unique_ptr<RSAPrivateKey> result(new RSAPrivateKey);
  result->key_.reset(EVP_PKEY_new());
  if (!result->key_ || !EVP_PKEY_set1_RSA(result->key_.get(), rsa_key.get())) {
    return nullptr;
  }

  return result;
}

}  // namespace crypto
