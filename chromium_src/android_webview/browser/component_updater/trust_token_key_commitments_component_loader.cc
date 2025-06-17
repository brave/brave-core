/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "android_webview/browser/component_updater/trust_token_key_commitments_component_loader.h"

#define LoadTrustTokenKeyCommitmentsComponent \
  LoadTrustTokenKeyCommitmentsComponent_ChromiumImpl

#include <android_webview/browser/component_updater/trust_token_key_commitments_component_loader.cc>
#undef LoadTrustTokenKeyCommitmentsComponent

namespace android_webview {

// We do not support TrustTokens aka PrivateStateTokens aka FeldgePst
void LoadTrustTokenKeyCommitmentsComponent(
    ComponentLoaderPolicyVector& policies) {}

}  // namespace android_webview
