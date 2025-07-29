/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/trust_token_key_commitments_component_installer.h"

#define RegisterTrustTokenKeyCommitmentsComponentIfTrustTokensEnabled \
  RegisterTrustTokenKeyCommitmentsComponentIfTrustTokensEnabled_ChromiumImpl

#include <chrome/browser/component_updater/trust_token_key_commitments_component_installer.cc>
#undef RegisterTrustTokenKeyCommitmentsComponentIfTrustTokensEnabled

namespace component_updater {

// We do not support TrustTokens aka PrivateStateTokens aka FeldgePst
void RegisterTrustTokenKeyCommitmentsComponentIfTrustTokensEnabled(
    ComponentUpdateService* cus) {}

}  // namespace component_updater
