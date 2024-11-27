/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/mixed_content_checker.h"

#include "base/strings/string_util.h"

#include "src/content/browser/renderer_host/mixed_content_checker.cc"

namespace content {

// static
bool MixedContentChecker::DoesOriginSchemeRestrictMixedContent(
    const url::Origin& origin) {
  if (origin.host().ends_with(".onion") &&
      (origin.scheme() == url::kHttpsScheme ||
       origin.scheme() == url::kHttpScheme ||
       origin.scheme() == url::kWsScheme ||
       origin.scheme() == url::kWssScheme)) {
    return true;
  }
  return ::content::DoesOriginSchemeRestrictMixedContent(origin);
}

}  // namespace content
