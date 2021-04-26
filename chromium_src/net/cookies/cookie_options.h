/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_COOKIES_COOKIE_OPTIONS_H_
#define BRAVE_CHROMIUM_SRC_NET_COOKIES_COOKIE_OPTIONS_H_

#include "base/optional.h"
#include "net/cookies/site_for_cookies.h"
#include "url/gurl.h"
#include "url/origin.h"

#define CookieOptions CookieOptions_ChromiumImpl
#include "../../../../net/cookies/cookie_options.h"
#undef CookieOptions

namespace net {

class CookieAccessDelegate;

class NET_EXPORT CookieOptions : public CookieOptions_ChromiumImpl {
 public:
  CookieOptions();
  CookieOptions(const CookieOptions&);
  CookieOptions(CookieOptions&&);
  ~CookieOptions();

  CookieOptions& operator=(const CookieOptions&);
  CookieOptions& operator=(CookieOptions&&);

  CookieOptions(const CookieOptions_ChromiumImpl&);  // NOLINT
  CookieOptions(CookieOptions_ChromiumImpl&&);       // NOLINT

  const net::SiteForCookies& site_for_cookies() const {
    return site_for_cookies_;
  }
  void set_site_for_cookies(const net::SiteForCookies& site_for_cookies) {
    site_for_cookies_ = site_for_cookies;
  }

  const base::Optional<url::Origin>& top_frame_origin() const {
    return top_frame_origin_;
  }
  void set_top_frame_origin(
      const base::Optional<url::Origin>& top_frame_origin) {
    top_frame_origin_ = top_frame_origin;
  }

  bool should_use_ephemeral_storage() const {
    return should_use_ephemeral_storage_;
  }
  void set_should_use_ephemeral_storage(bool should_use_ephemeral_storage) {
    should_use_ephemeral_storage_ = should_use_ephemeral_storage;
  }

 private:
  net::SiteForCookies site_for_cookies_;
  base::Optional<url::Origin> top_frame_origin_;
  bool should_use_ephemeral_storage_ = false;
};

// Fills ephemeral storage-specific parameters.
void NET_EXPORT
FillEphemeralStorageParams(const GURL& url,
                           const SiteForCookies& site_for_cookies,
                           const base::Optional<url::Origin>& top_frame_origin,
                           const CookieAccessDelegate* cookie_access_delegate,
                           CookieOptions* cookie_options);

}  // namespace net

#endif  // BRAVE_CHROMIUM_SRC_NET_COOKIES_COOKIE_OPTIONS_H_
