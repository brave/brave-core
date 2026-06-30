/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/connection_allowlist_gating.h"

#include "content/public/common/url_constants.h"
#include "url/gurl.h"

#define ResponseEnablesConnectionAllowlistsOriginTrial \
  ResponseEnablesConnectionAllowlistsOriginTrial_ChromiumImpl

#include <content/browser/connection_allowlist_gating.cc>

#undef ResponseEnablesConnectionAllowlistsOriginTrial

namespace content {

bool ResponseEnablesConnectionAllowlistsOriginTrial(
    const GURL& request_url,
    const net::HttpResponseHeaders* response_headers) {
  // Browser-internal WebUI pages are first-party and trusted by construction.
  // They do not enroll in an origin trial to use a browser feature on their
  // own documents, so exempt the local scheme from the origin-trial token
  // requirement. This lets Brave's on-device model worker WebUIs honor an
  // injected (enforced, empty) Connection-Allowlist without flipping the global
  // kOverrideConnectionAllowlistOriginTrial feature. Real web origins still go
  // through the normal feature-or-token gate unchanged.
  if (request_url.SchemeIs(kChromeUIUntrustedScheme)) {
    return true;
  }
  return ResponseEnablesConnectionAllowlistsOriginTrial_ChromiumImpl(
      request_url, response_headers);
}

}  // namespace content
