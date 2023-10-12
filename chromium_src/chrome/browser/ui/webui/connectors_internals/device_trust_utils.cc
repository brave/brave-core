/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/crypto/signature_verifier.h"  // IWYU pragma: export

#define ECDSA_SHA256 \
  ECDSA_SHA256:      \
  case crypto::SignatureVerifier::ECDSA_SHA384

#include "src/chrome/browser/ui/webui/connectors_internals/device_trust_utils.cc"
#undef ECDSA_SHA256
