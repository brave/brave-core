/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CRYPTO_RSA_PRIVATE_KEY_H_
#define BRAVE_CHROMIUM_SRC_CRYPTO_RSA_PRIVATE_KEY_H_

#define ExportPrivateKey                                           \
  Unused();                                                        \
  static std::unique_ptr<RSAPrivateKey> Create(uint16_t num_bits); \
  bool ExportPrivateKey

#include <crypto/rsa_private_key.h>  // IWYU pragma: export

#undef ExportPrivateKey

#endif  // BRAVE_CHROMIUM_SRC_CRYPTO_RSA_PRIVATE_KEY_H_
