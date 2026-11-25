/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "services/network/cookie_settings.h"

#include <memory>
#include <optional>
#include <utility>

#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_access_result.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_partition_key.h"
#include "net/cookies/cookie_setting_override.h"
#include "net/cookies/site_for_cookies.h"
#include "url/origin.h"

#include <services/network/cookie_settings_unittest.cc>

namespace network {
namespace {

// Ephemeral storage allows a cookie's access up front. A prefetch's
// `kForceDisablePartitionedCookies` must still turn down partitioned cookies.
TEST_F(CookieSettingsTest, EphemeralAccessibleHonorsDisablePartitioned) {
  CookieSettings settings;
  settings.set_block_third_party_cookies(false);
  const GURL url(kURL);
  const url::Origin top_frame_origin = url::Origin::Create(GURL(kOtherURL));
  std::unique_ptr<net::CanonicalCookie> partitioned_cookie =
      MakeCanonicalCookie(
          "__Host-partitioned", kURL,
          net::CookiePartitionKey::FromURLForTesting(GURL(kOtherURL)));
  std::unique_ptr<net::CanonicalCookie> unpartitioned_cookie =
      MakeCanonicalSameSiteNoneCookie("__Host-unpartitioned", kURL);
  const net::CookieSettingOverrides overrides{
      net::CookieSettingOverride::kForceDisablePartitionedCookies};
  net::CookieInclusionStatus status;

  EXPECT_TRUE(settings.IsEphemeralCookieAccessible(
      *partitioned_cookie, url, net::SiteForCookies(), top_frame_origin,
      net::CookieSettingOverrides(), &status));
  EXPECT_TRUE(settings.IsEphemeralCookieAccessible(
      *unpartitioned_cookie, url, net::SiteForCookies(), top_frame_origin,
      overrides, &status));
  EXPECT_FALSE(settings.IsEphemeralCookieAccessible(
      *partitioned_cookie, url, net::SiteForCookies(), top_frame_origin,
      overrides, &status));
}

TEST_F(CookieSettingsTest, EphemeralAnnotateHonorsDisablePartitioned) {
  CookieSettings settings;
  settings.set_block_third_party_cookies(false);
  const GURL url(kURL);
  const url::Origin top_frame_origin = url::Origin::Create(GURL(kOtherURL));
  std::unique_ptr<net::CanonicalCookie> partitioned_cookie =
      MakeCanonicalCookie(
          "__Host-partitioned", kURL,
          net::CookiePartitionKey::FromURLForTesting(GURL(kOtherURL)));

  auto count_included = [&](net::CookieSettingOverrides overrides) {
    net::CookieAccessResultList included;
    net::CookieAccessResultList excluded;
    included.emplace_back(
        *partitioned_cookie,
        net::CookieAccessResult(net::CookieInclusionStatus()));
    settings.AnnotateAndMoveUserBlockedEphemeralCookies(
        url, net::SiteForCookies(), &top_frame_origin, overrides, included,
        excluded);
    return included.size();
  };

  EXPECT_EQ(1u, count_included(net::CookieSettingOverrides()));
  EXPECT_EQ(0u,
            count_included(net::CookieSettingOverrides{
                net::CookieSettingOverride::kForceDisablePartitionedCookies}));
}

}  // namespace
}  // namespace network
