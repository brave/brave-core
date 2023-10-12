/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/crypto/signature_verifier.h"  // IWYU pragma: export

#define ECDSA_SHA256       \
  ECDSA_SHA384:            \
  pkey_type = EVP_PKEY_EC; \
  digest = EVP_sha384();   \
  break;                   \
  case ECDSA_SHA256

#include "src/crypto/signature_verifier.cc"
#undef ECDSA_SHA256
