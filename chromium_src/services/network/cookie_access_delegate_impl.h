/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_ACCESS_DELEGATE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_ACCESS_DELEGATE_IMPL_H_

#include "net/cookies/cookie_access_delegate.h"

#define ShouldTreatUrlAsTrustworthy                                        \
  NotUsed() const override;                                                \
  bool ShouldUseEphemeralStorage(                                          \
      const GURL& url, const net::SiteForCookies& site_for_cookies,        \
      const base::Optional<url::Origin>& top_frame_origin) const override; \
  bool ShouldTreatUrlAsTrustworthy

#include "../../../../services/network/cookie_access_delegate_impl.h"

#undef ShouldTreatUrlAsTrustworthy

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_ACCESS_DELEGATE_IMPL_H_
