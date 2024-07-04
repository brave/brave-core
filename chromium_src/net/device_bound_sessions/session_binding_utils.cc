/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/device_bound_sessions/session_binding_utils.h"

#define RSA_PKCS1_SHA1 \
  ECDSA_SHA384:        \
  return "SHA384";     \
  case crypto::SignatureVerifier::RSA_PKCS1_SHA1

#include "src/net/device_bound_sessions/session_binding_utils.cc"

#undef RSA_PKCS1_SHA1
