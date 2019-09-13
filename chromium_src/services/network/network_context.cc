/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/network_context.h"

GURL GetURLForCookieAccess(const net::URLRequest& request) {
  if (!request.site_for_cookies().is_empty())
    return request.site_for_cookies();

  if (request.network_isolation_key().IsFullyPopulated()) {
    GURL origin(request.network_isolation_key().ToString());
    if (origin.is_valid())
      return origin;
  }

  if (request.top_frame_origin().has_value())
    return request.top_frame_origin()->GetURL();

  return GURL();
}

#define BRAVE_ON_CAN_GET_COOKIES_INTERNAL \
network_context_->cookie_manager() \
                ->cookie_settings() \
                .IsCookieAccessAllowed( \
                    request.url(), \
                    GetURLForCookieAccess(request)) \
                    &&

#define BRAVE_ON_CAN_SET_COOKIES_INTERNAL BRAVE_ON_CAN_GET_COOKIES_INTERNAL

#include "../../../../services/network/network_context.cc"  // NOLINT
#undef BRAVE_ON_CAN_GET_COOKIES_INTERNAL
#undef BRAVE_ON_CAN_SET_COOKIES_INTERNAL
