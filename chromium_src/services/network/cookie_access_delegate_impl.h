/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_ACCESS_DELEGATE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_ACCESS_DELEGATE_IMPL_H_

#include <optional>

#include "net/cookies/cookie_access_delegate.h"

#define ShouldTreatUrlAsTrustworthy                                           \
  NotUsed() const override;                                                   \
  bool ShouldUseEphemeralStorage(                                             \
      const GURL& url, const net::SiteForCookies& site_for_cookies,           \
      base::optional_ref<const url::Origin> top_frame_origin) const override; \
  bool ShouldTreatUrlAsTrustworthy

#include "src/services/network/cookie_access_delegate_impl.h"  // IWYU pragma: export

#undef ShouldTreatUrlAsTrustworthy

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_COOKIE_ACCESS_DELEGATE_IMPL_H_
