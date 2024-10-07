/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "crypto/signature_verifier.h"

#define RSA_PSS_SHA256 \
  RSA_PSS_SHA256:      \
  case SignatureVerifier::SignatureAlgorithm::ECDSA_SHA384
#include "src/crypto/unexportable_key_software_unsecure.cc"
#undef RSA_PSS_SHA256
