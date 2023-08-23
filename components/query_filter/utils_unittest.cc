// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
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
}
