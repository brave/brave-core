/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/cookie_manager.h"
#include "services/network/restricted_cookie_manager.h"

#define BRAVE_DELETIONFILTERTOINFO       \
  delete_info.ephemeral_storage_domain = \
      std::move(filter->ephemeral_storage_domain);

#define GetCookieList GetCookieList_ChromiumImpl
#define SetCanonicalCookie SetCanonicalCookie_ChromiumImpl

#include "../../../../services/network/cookie_manager.cc"

#undef GetCookieList
#undef SetCanonicalCookie

namespace network {

void CookieManager::GetCookieList(const GURL& url,
                                  const net::CookieOptions& cookie_options,
                                  GetCookieListCallback callback) {
  if (!cookie_options.should_use_ephemeral_storage() &&
      cookie_settings_.ShouldUseEphemeralStorage(
          url, cookie_options.site_for_cookies().RepresentativeUrl(),
          cookie_options.top_frame_origin())) {
    auto ephemeral_cookie_options = cookie_options;
    ephemeral_cookie_options.set_should_use_ephemeral_storage(true);
    cookie_store_->GetCookieListWithOptionsAsync(url, ephemeral_cookie_options,
                                                 std::move(callback));
    return;
  }

  GetCookieList_ChromiumImpl(url, cookie_options, std::move(callback));
}

void CookieManager::SetCanonicalCookie(const net::CanonicalCookie& cookie,
                                       const GURL& source_url,
                                       const net::CookieOptions& cookie_options,
                                       SetCanonicalCookieCallback callback) {
  if (!cookie_options.should_use_ephemeral_storage() &&
      cookie_settings_.ShouldUseEphemeralStorage(
          source_url, cookie_options.site_for_cookies().RepresentativeUrl(),
          cookie_options.top_frame_origin())) {
    auto ephemeral_cookie_options = cookie_options;
    ephemeral_cookie_options.set_should_use_ephemeral_storage(true);
    cookie_store_->SetCanonicalCookieAsync(
        std::make_unique<net::CanonicalCookie>(cookie), source_url,
        ephemeral_cookie_options, std::move(callback));
    return;
  }

  SetCanonicalCookie_ChromiumImpl(cookie, source_url, cookie_options,
                                  std::move(callback));
}

}  // namespace network
