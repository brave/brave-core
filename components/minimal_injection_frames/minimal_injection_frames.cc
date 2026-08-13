/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/minimal_injection_frames/minimal_injection_frames.h"

#include <string_view>

#include "base/containers/fixed_flat_set.h"
#include "url/origin.h"

namespace brave {

namespace {

// Serialized origins, matched against url::Origin::Serialize(). An entry that
// is not a valid serialized origin (no scheme, a trailing slash, a path, a
// default port spelled out) silently matches nothing, so keep the format exact.
constexpr auto kMinimalInjectionOrigins =
    base::MakeFixedFlatSet<std::string_view>({
        "https://challenges.cloudflare.com",
    });

}  // namespace

bool IsMinimalInjectionFrame(const url::Origin& origin) {
  // Serialize() is "null" for opaque origins, which no entry can match.
  return kMinimalInjectionOrigins.contains(origin.Serialize());
}

}  // namespace brave
