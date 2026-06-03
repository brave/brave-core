/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Brave disables Chromium's cookie IPC cache optimization (because Ephemeral
// Storage can switch the cookie backend at runtime), which means
// document.cookie always uses IPC.  When a test sets a cookie via
// document.cookie and immediately queries the backend cookie store through a
// *different* Mojo pipe (CookieManager::GetAllCookies), there is no ordering
// guarantee between the two pipes.  Upstream Chromium sees this as a ~0.6 %
// flake; Brave amplifies it because every cookie read/write goes through IPC.
//
// Fix: after ExecJs("set<Type>()"), verify the data is accessible via
// EvalJs("has<Type>()").  For cookies this forces a round-trip through
// RestrictedCookieManager → CookieStore, serialising with the prior
// SetCanonicalCookieAsync on the same CookieMonster task runner and
// guaranteeing the cookie is committed before any subsequent GetAllCookies.

#define SetDataForType SetDataForType_ChromiumImpl

#include <components/browsing_data/content/browsing_data_test_util.cc>  // IWYU pragma: export

#undef SetDataForType

namespace browsing_data_test_util {

void SetDataForType(const std::string& type,
                    content::WebContents* web_contents) {
  SetDataForType_ChromiumImpl(type, web_contents);
  // Round-trip through RestrictedCookieManager to synchronise the CookieStore.
  EXPECT_TRUE(HasDataForType(type, web_contents));
}

void SetDataForType(const std::string& type,
                    content::RenderFrameHost* render_frame_host) {
  SetDataForType_ChromiumImpl(type, render_frame_host);
}

}  // namespace browsing_data_test_util
