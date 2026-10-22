/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/update_client/privacy_preserving_protocol_handler.h"

#define EnabledCupSigning EnabledCupSigning_Unused
#define ProtocolHandlerFactoryJSON PrivacyPreservingProtocolHandlerFactory
#include <chrome/updater/configurator.cc>
#undef ProtocolHandlerFactoryJSON
#undef EnabledCupSigning

namespace updater {

// Our CUP key is still ECDSA, but upstream's verification path now only
// accepts ML-DSA-44 signatures (see
// patches/components-client_update_protocol-cup.cc.patch), so a signed
// response can never validate. Disable CUP for the updater until we have an
// ML-DSA-44 key of our own.
bool Configurator::EnabledCupSigning() const {
  return false;
}

}  // namespace updater
