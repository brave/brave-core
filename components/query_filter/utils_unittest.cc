// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/utils.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

TEST(BraveQueryFilter, FilterQueryTrackers) {
  // Expect filtering `gclid` param when cross origin `initiator_url`
  // and `redirect_source_url`
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL("https://brave.com"),
                GURL("https://test.com/?gclid=123"), "GET", false),
            GURL("https://test.com/"));
  // Expect filtering `gclid` param when cross origin `redirect_source_url`
  // and same origin `initiator_url`
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://test.com"), GURL("https://brave.com"),
                GURL("https://test.com/?gclid=123"), "GET", false),
            GURL("https://test.com/"));
  // Expect filtering `fbclid` param when cross origin `initiator_url`
  // and invalid `redirect_source_url`
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL(),
                GURL("https://test.com/?fbclid=123"), "GET", false),
            GURL("https://test.com/"));
  // Expect filtering `fbclid` param when cross origin `initiator_url`
  // and invalid `redirect_source_url`
  // (even though `internal_redirect=true`)
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL(),
                GURL("https://test.com/?fbclid=123"), "GET", false),
            GURL("https://test.com/"));
  // Expect filtering `fbclid` param when cross origin `initiator_url`
  // and invalid `redirect_source_url` even though `internal_redirect` == true
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL(),
                GURL("https://test.com/?fbclid=123"), "GET", false),
            GURL("https://test.com/"));
  // Expect filtering `mkt_tok` param when cross origin `redirect_source_url`
  // and invalid `initiator_url`
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?mkt_tok=123"), "GET", false),
            GURL("https://test.com/"));
  // Test filtering `gclid` param and leaving other params
  EXPECT_EQ(
      query_filter::MaybeApplyQueryStringFilter(
          GURL("https://brave.com"), GURL(),
          GURL("https://test.com/?gclid=123&unsubscribe=123"), "GET", false),
      GURL("https://test.com/?unsubscribe=123"));
  // Test filtering `gclid` param and leaving other params
  EXPECT_EQ(
      query_filter::MaybeApplyQueryStringFilter(
          GURL(), GURL("https://brave.com"),
          GURL("https://test.com/?gclid=123&Unsubscribe=123"), "GET", false),
      GURL("https://test.com/?Unsubscribe=123"));
  // Test nil when nothing is needed to be filtered
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://brave.com"),
      GURL("https://test.com/"), "GET", false));
  // Test nothing is filtered with invalid request url
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://brave.com"), GURL(), "GET",
      false));
  // Only `GET` requests are filtered
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://brave.com"),
      GURL("https://test.com/?gclid=123"), "POST", false));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://brave.com"),
      GURL("https://test.com/?gclid=123"), "PATH", false));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://brave.com"),
      GURL("https://test.com/?gclid=123"), "HEAD", false));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://brave.com"),
      GURL("https://test.com/?gclid=123"), "PUT", false));
  // Test nothing is filtered with same origin `initiator_url`
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://test.com"), GURL(), GURL("https://test.com/?gclid=123"),
      "GET", false));
  // Test nothing is filtered with same origin `redirect_source_url`
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://test.com"), GURL("https://test.com/?gclid=123"),
      "GET", false));
  // Test nothing is filtered with same origin `redirect_source_url`
  // and cross origin `initiator_url`
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://test.com"),
      GURL("https://test.com/?gclid=123"), "GET", false));
  // Test nothing is filtered with `internal_redirect=true`
  // (even though `redirect_source_url` and `initiator_url` are cross origin)
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://brave.com"),
      GURL("https://test.com/?gclid=123"), "GET", true));
  // Don't filter exempted hostnames
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL(),
      GURL("https://urldefense.com/v3/__https://www.portainer.io/hs/"
           "preferences-center/en/"
           "direct?utm_campaign=XNF&utm_source=hs_automation&_hsenc=p2&_hsmi="
           "26__;!!MlclJBHn!0eDf-z$"),
      "GET", false));
}

TEST(BraveQueryFilter, IsScopedTracker) {
  auto trackers = std::map<std::string_view, std::vector<std::string_view>>({
      {"igshid", {"instagram.com"}},
      {"ref_src", {"twitter.com", "x.com", "y.com"}},
      {"sample1", {"", " ", "brave.com", ""}},
      {"sample2", {" "}},
      {"sample3", {""}},
      {"sample4", {}},
  });

  // Normal case for a parameter that's not on the list
  EXPECT_FALSE(query_filter::IsScopedTrackerForTesting(
      "t", "https://twitter.com/", trackers));

  // Normal case with a single domain
  EXPECT_TRUE(query_filter::IsScopedTrackerForTesting(
      "igshid", "https://instagram.com/", trackers));
  EXPECT_TRUE(query_filter::IsScopedTrackerForTesting(
      "igshid", "http://www.instagram.com/", trackers));
  EXPECT_FALSE(query_filter::IsScopedTrackerForTesting(
      "igshid", "https://example.com/", trackers));

  // Normal case with more than one domain
  EXPECT_TRUE(query_filter::IsScopedTrackerForTesting(
      "ref_src", "https://twitter.com/", trackers));
  EXPECT_TRUE(query_filter::IsScopedTrackerForTesting(
      "ref_src", "https://x.com/", trackers));
  EXPECT_TRUE(query_filter::IsScopedTrackerForTesting(
      "ref_src", "https://y.com/", trackers));
  EXPECT_FALSE(query_filter::IsScopedTrackerForTesting(
      "ref_src", "https://z.com/", trackers));

  // Edge cases
  EXPECT_TRUE(query_filter::IsScopedTrackerForTesting(
      "sample1", "https://brave.com/", trackers));
  EXPECT_FALSE(query_filter::IsScopedTrackerForTesting(
      "sample1", "https://example.com/", trackers));
  EXPECT_FALSE(query_filter::IsScopedTrackerForTesting(
      "sample2", "https://brave.com/", trackers));
  EXPECT_FALSE(query_filter::IsScopedTrackerForTesting(
      "sample3", "https://brave.com/", trackers));
  EXPECT_FALSE(query_filter::IsScopedTrackerForTesting(
      "sample4", "https://brave.com/", trackers));
}
