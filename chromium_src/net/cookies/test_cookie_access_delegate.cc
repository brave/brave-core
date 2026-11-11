/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/cookies/test_cookie_access_delegate.h"

#include <net/cookies/test_cookie_access_delegate.cc>

namespace net {

// Tests using this delegate check upstream cookie semantics, which have no
// ephemeral storage. The base implementation is NOTREACHED().
bool TestCookieAccessDelegate::ShouldUseEphemeralStorage(
    const GURL& url,
    const SiteForCookies& site_for_cookies,
    base::optional_ref<const url::Origin> top_frame_origin) const {
  return false;
}

}  // namespace net
