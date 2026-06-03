/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/https_upgrades_navigation_throttle.h"
#include "net/dns/dns_http_attempt.h"
#include "url/gurl.h"

namespace {
// The same as in chromium_src/.../https_only_mode_blocking_page.cc
constexpr char kBraveLearnMoreLink[] =
    "https://support.brave.app/hc/en-us/articles/15513090104717";
}  // namespace

// We changed the `Learn more` link on the interstitial page. It checks the link
// and provides the result for upstream's EXPECT_EQ.
#define GetQuery() spec() == kBraveLearnMoreLink ? "p=first_mode" : "invalid"

// We can't use a zero timeout because DomainBlockNavigationThrottle defers the
// redirect, which leads to the navigation request being canceled too early (due
// to timeout), increasing the timeout respects adblock's checking.
#define set_timeout_for_testing(timeout) \
  set_timeout_for_testing(timeout.is_zero() ? base::Seconds(1) : timeout)

#include <chrome/browser/ssl/https_upgrades_browsertest.cc>

#undef GetQuery
#undef set_timeout_for_testing
