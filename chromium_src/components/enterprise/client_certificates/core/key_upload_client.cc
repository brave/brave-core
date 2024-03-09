/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/enterprise/client_certificates/core/key_upload_client.h"

#include "crypto/signature_verifier.h"

// Adds a fallthrough case for `AlgorithmToType` to handle `ECDSA_SHA384` as
// well.
#define ECDSA_SHA256 \
  ECDSA_SHA256:      \
  case crypto::SignatureVerifier::ECDSA_SHA384

#include "src/components/enterprise/client_certificates/core/key_upload_client.cc"

#undef ECDSA_SHA256
