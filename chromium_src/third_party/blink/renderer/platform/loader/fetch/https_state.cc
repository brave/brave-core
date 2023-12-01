/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "third_party/blink/renderer/platform/loader/fetch/https_state.h"

#define CalculateHttpsState CalculateHttpsState_ChromiumImpl

#include "src/third_party/blink/renderer/platform/loader/fetch/https_state.cc"

#undef CalculateHttpsState

namespace blink {

HttpsState CalculateHttpsState(const SecurityOrigin* security_origin,
                               std::optional<HttpsState> parent_https_state) {
  if (security_origin && (security_origin->Protocol() == "http" &&
                          security_origin->Host().EndsWith(".onion"))) {
    // MixedContentChecker::ShouldAutoupgrade upgrades resource only on the
    // kModern pages. Return it for .onion as for https.
    return HttpsState::kModern;
  }

  return CalculateHttpsState_ChromiumImpl(security_origin, parent_https_state);
}

}  // namespace blink
