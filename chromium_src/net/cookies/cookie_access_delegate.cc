/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/cookies/cookie_access_delegate.h"

#include <optional>

#include "base/notreached.h"

#include <net/cookies/cookie_access_delegate.cc>

namespace net {

bool CookieAccessDelegate::NotUsed() const {
  return false;
}

bool CookieAccessDelegate::ShouldUseEphemeralStorage(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    base::optional_ref<const url::Origin> top_frame_origin) const {
  NOTREACHED() << "Should be overridden";
}

}  // namespace net
