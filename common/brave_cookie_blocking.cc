/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_cookie_blocking.h"

#include "brave/common/shield_exceptions.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

bool ShouldBlockCookie(bool allow_brave_shields,
                       bool allow_1p_cookies,
                       bool allow_3p_cookies,
                       const GURL& primary_url,
                       const GURL& url,
                       bool allow_google_auth) {
  if (primary_url.SchemeIs("chrome-extension")) {
    return false;
  }

  if (!allow_brave_shields) {
    return false;
  }

  // If 1p cookies are not allowed, then we just want to block everything.
  if (!allow_1p_cookies) {
    return true;
  }

  // If 3p is allowed, we have nothing extra to block
  if (allow_3p_cookies) {
    return false;
  }

  // If it is whitelisted, we shouldn't block
  if (brave::IsWhitelistedCookieException(
          primary_url, url, allow_google_auth)) {
    return false;
  }

  // Same TLD+1 whouldn't set the referrer
  return !SameDomainOrHost(
      url,
      primary_url,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}
