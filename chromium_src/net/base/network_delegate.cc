/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/base/network_delegate.h"

#include "net/url_request/url_request.h"

namespace net {
bool NetworkDelegate::OnCanGetCookiesWithoutEphemeralCookies(
    const URLRequest& request,
    bool allowed_from_caller) {
  return OnCanGetCookies(request, allowed_from_caller);
}

bool NetworkDelegate::OnCanSetCookieWithoutEphemeralCookies(
    const URLRequest& request,
    const CanonicalCookie& cookie,
    CookieOptions* options,
    bool allowed_from_caller) {
  return OnCanSetCookie(request, cookie, options, allowed_from_caller);
}
}  // namespace net

#include "../../../../net/base/network_delegate.cc"
