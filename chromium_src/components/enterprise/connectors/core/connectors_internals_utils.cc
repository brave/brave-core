/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/enterprise/connectors/core/connectors_internals_utils.h"

#include "crypto/signature_verifier.h"

// Adds a case for `SignatureAlgorithm` to handle `ECDSA_SHA384` as well.
#define ECDSA_SHA256 \
  ECDSA_SHA256:      \
  case crypto::SignatureVerifier::ECDSA_SHA384

#include <components/enterprise/connectors/core/connectors_internals_utils.cc>
#undef ECDSA_SHA256
