/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "services/network/cookie_access_delegate_impl.h"
#include "src/services/network/cookie_access_delegate_impl.cc"

namespace network {

bool CookieAccessDelegateImpl::NotUsed() const {
  return false;
}

bool CookieAccessDelegateImpl::ShouldUseEphemeralStorage(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    base::optional_ref<const url::Origin> top_frame_origin) const {
  if (!cookie_settings_) {
    return false;
  }
  return cookie_settings_->ShouldUseEphemeralStorage(url, site_for_cookies,
                                                     top_frame_origin);
}

}  // namespace network
