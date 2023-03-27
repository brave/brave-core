/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_CONNECTORS_INTERNALS_DEVICE_TRUST_UTILS_CC_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_CONNECTORS_INTERNALS_DEVICE_TRUST_UTILS_CC_

#include "brave/chromium_src/crypto/signature_verifier.h"  // IWYU pragma: export

#define ECDSA_SHA256 \
  ECDSA_SHA256:      \
  case crypto::SignatureVerifier::ECDSA_SHA384

#include "src/chrome/browser/ui/webui/connectors_internals/device_trust_utils.cc"

#undef ECDSA_SHA256

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_CONNECTORS_INTERNALS_DEVICE_TRUST_UTILS_CC_
