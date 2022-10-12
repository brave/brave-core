/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/mixed_content_checker.h"

#define BRAVE_MIXED_CONTENT_CHECKER_IS_MIXED_CONTENT              \
  if (auto result = IsMixedContentForOnion(security_origin, url)) \
    return *result;

#include "src/third_party/blink/renderer/core/loader/mixed_content_checker.cc"

#undef BRAVE_MIXED_CONTENT_CHECKER_IS_MIXED_CONTENT

namespace blink {

// static
absl::optional<bool> MixedContentChecker::IsMixedContentForOnion(
    const SecurityOrigin* security_origin,
    const KURL& resource_url) {
  constexpr const char kOnion[] = ".onion";

  if (!security_origin->Host().EndsWith(kOnion))
    return absl::nullopt;

  if (resource_url.Host().EndsWith(kOnion)) {
    // onion -> onion: not blocked
    return false;
  }
  // Treat .onions as https://
  // onion -> https: not blocked
  // onion -> http: blocked
  return IsMixedContent("https", resource_url);
}

}  // namespace blink
