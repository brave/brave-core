/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <tuple>

// The upstream test sets a cookie via document.cookie (CookieJar mojo pipe,
// async with AsyncSetCookie) then immediately reads it via cookieStore.get()
// (CookieStore API, separate mojo pipe). These two APIs use independent
// RestrictedCookieManager pipes, so there is no ordering guarantee for the
// underlying CookieMonster operations. Brave's ephemeral storage checks in
// FillEphemeralStorageParams and IsEphemeralCookieAccessible add processing
// overhead that widens this race window enough to cause intermittent failures.
//
// Fix: redefine the test to read document.cookie after setting it. Since
// document.cookie getter is a sync IPC on the same pipe as the setter, it
// forces the network service to commit the set before returning. After that,
// cookieStore.get() on a separate pipe reliably finds the cookie.

#define CookieStoreUsesEffectiveSameSiteForUnspecifiedCookies \
  DISABLED_CookieStoreUsesEffectiveSameSiteForUnspecifiedCookies

#include <chrome/browser/net/cookie_store_samesite_browsertest.cc>

#undef CookieStoreUsesEffectiveSameSiteForUnspecifiedCookies

IN_PROC_BROWSER_TEST_P(CookieStoreSameSiteTest,
                       CookieStoreUsesEffectiveSameSiteForUnspecifiedCookies) {
  NavigateToPageWithFrame("a.test");

  EXPECT_TRUE(content::ExecJs(GetFrame(),
                              "document.cookie='unspecified-cookie=value'"));

  // Synchronize: reading document.cookie is a sync IPC on the CookieJar mojo
  // pipe — the same pipe the setter used. Mojo message ordering on a single
  // pipe guarantees the async set is committed before this getter returns.
  std::ignore = content::EvalJs(GetFrame(), "document.cookie");

  std::string sameSite = GetCookieSameSite("unspecified-cookie");
  if (HasNonLegacySameSiteAccessSemantics()) {
    EXPECT_EQ("lax", sameSite);
  } else {
    EXPECT_EQ("none", sameSite);
  }
}
