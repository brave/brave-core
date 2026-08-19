/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/api/cookies/cookies_api.h"

#include <chrome/browser/extensions/api/cookies/cookies_api.cc>

namespace extensions {

// static
void OnCookieChangeExposeForTesting::CallOnCookieChangeForOtr(
    CookiesAPI* cookies_api) {
  cookies_api->cookies_event_router_->OnCookieChange(true,
                                                     net::CookieChangeInfo());
}

}  // namespace extensions
