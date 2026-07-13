/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "third_party/blink/renderer/platform/loader/fetch/https_state.h"

#define CalculateHttpsState CalculateHttpsState_ChromiumImpl

#include <third_party/blink/renderer/platform/loader/fetch/https_state.cc>

#undef CalculateHttpsState

namespace blink {

HttpsState CalculateHttpsState(const SecurityOrigin* security_origin,
                               std::optional<HttpsState> parent_https_state) {
  // Use the precursor origin so that opaque origins (e.g. an .onion page
  // sandboxed via `Content-Security-Policy: sandbox`) are still recognized as
  // .onion. Reading the opaque origin's Protocol()/Host() directly returns
  // empty strings, which would leave the page as kNone and let insecure
  // (mixed) content through instead of treating .onion as https.
  const SecurityOrigin* tuple_origin =
      security_origin ? security_origin->GetOriginOrPrecursorOriginIfOpaque()
                      : nullptr;
  if (tuple_origin && (tuple_origin->Protocol() == "http" &&
                       tuple_origin->Host().ends_with(".onion"))) {
    // MixedContentChecker::ShouldAutoupgrade upgrades resource only on the
    // kModern pages. Return it for .onion as for https.
    return HttpsState::kModern;
  }

  return CalculateHttpsState_ChromiumImpl(security_origin, parent_https_state);
}

}  // namespace blink
