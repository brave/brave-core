/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_COOKIES_COOKIE_ACCESS_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_NET_COOKIES_COOKIE_ACCESS_DELEGATE_H_

#include <optional>

#include "base/types/optional_ref.h"
#include "net/cookies/cookie_setting_override.h"
#include "net/cookies/site_for_cookies.h"
#include "url/gurl.h"
#include "url/origin.h"

#define ShouldTreatUrlAsTrustworthy                                  \
  NotUsed() const;                                                   \
  virtual bool ShouldUseEphemeralStorage(                            \
      const GURL& url, const net::SiteForCookies& site_for_cookies,  \
      base::optional_ref<const url::Origin> top_frame_origin) const; \
  virtual bool ShouldTreatUrlAsTrustworthy

#include "src/net/cookies/cookie_access_delegate.h"  // IWYU pragma: export

#undef ShouldTreatUrlAsTrustworthy

#endif  // BRAVE_CHROMIUM_SRC_NET_COOKIES_COOKIE_ACCESS_DELEGATE_H_
