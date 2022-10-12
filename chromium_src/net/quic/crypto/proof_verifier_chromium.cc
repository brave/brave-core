/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_state.h"

#define ShouldSSLErrorsBeFatal(host) \
  ShouldSSLErrorsBeFatal(proof_verifier_->network_anonymization_key_, host)

#include "src/net/quic/crypto/proof_verifier_chromium.cc"

#undef ShouldSSLErrorsBeFatal
