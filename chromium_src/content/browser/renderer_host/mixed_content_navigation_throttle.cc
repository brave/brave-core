/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/mixed_content_navigation_throttle.h"

#include "base/strings/string_util.h"

#include "src/content/browser/renderer_host/mixed_content_navigation_throttle.cc"

namespace content {

// static
bool MixedContentNavigationThrottle::DoesOriginSchemeRestrictMixedContent(
    const url::Origin& origin) {
  constexpr const char kOnion[] = ".onion";
  if (base::EndsWith(origin.host(), kOnion) &&
      (origin.scheme() == url::kHttpsScheme ||
       origin.scheme() == url::kHttpScheme)) {
    return true;
  }
  return ::content::DoesOriginSchemeRestrictMixedContent(origin);
}

}  // namespace content
