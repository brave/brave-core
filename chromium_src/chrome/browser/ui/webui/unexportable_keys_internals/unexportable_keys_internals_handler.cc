/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/crypto/signature_verifier.h"

#define ECDSA_SHA256     \
  ECDSA_SHA384:          \
  return "ECDSA_SHA384"; \
  case crypto::SignatureVerifier::SignatureAlgorithm::ECDSA_SHA256

#include <chrome/browser/ui/webui/unexportable_keys_internals/unexportable_keys_internals_handler.cc>
#undef ECDSA_SHA256
