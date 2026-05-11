// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/utils.h"

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "brave/components/query_filter/browser/query_filter_data.h"
#include "brave/components/query_filter/browser/test_support/query_filter_test_helper.h"
#include "brave/components/query_filter/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class BraveQueryFilter_ComponentEnabled : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        query_filter::features::kQueryFilterComponent);
    filter_test_rules_.emplace();
  }

  void TearDown() override { ResetRules(); }

  void ResetRules() {
    if (filter_test_rules_.has_value()) {
      filter_test_rules_.reset();
    }
  }

  void UpdateRules(std::string_view json) {
    ASSERT_TRUE(filter_test_rules_);
    ASSERT_TRUE(filter_test_rules_->UpdateRules(json));
  }

  query_filter::QueryFilterData* instance() const {
    return query_filter::QueryFilterData::GetInstance();
  }

 private:
  std::optional<query_filter::test::ScopedTestingQueryFilterRules>
      filter_test_rules_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveQueryFilter_ComponentEnabled, FilterQueryTrackers) {
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

TEST_F(BraveQueryFilter_ComponentEnabled, ScopedQueryTrackingTest) {
  // Update with the new set of rules.
  UpdateRules(
      R"json([
               {
                   "include": [
                       "*://*.instagram.com/*",
                       "*://instagram.com/*"
                   ],
                   "exclude": [],
                   "params": [
                       "igshid"
                   ]
               },
               {
                   "include": [
                       "*://*twitter.com",
                       "*://*x.com",
                       "*://*y.com"
                   ],
                   "exclude": [],
                   "params": [
                       "ref_src"
                   ]
               },
               {
                   "include": [
                       "*://brave.com"
                   ],
                   "exclude": [],
                   "params": ["sample1"]
               },
               {
                   "include": [""],
                   "exclude": [],
                   "params": ["sample2", "sample3", "sample4"]
               },
               {
                   "include": ["*://*.facebook.com/*"],
                   "exclude": ["*://*theshining.com/*"],
                   "params": ["evil"]
               }
               ])json");

  // Normal case for a parameter that's not on the list
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL(), GURL("https://twitter.com/?t=123"),
      "GET", false));

  // Normal case with a single domain
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL(),
                GURL("https://instagram.com/?igshid=123"), "GET", false),
            GURL("https://instagram.com/"));
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL(),
                GURL("http://www.instagram.com/?igshid=123"), "GET", false),
            GURL("http://www.instagram.com/"));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL(),
      GURL("https://example.com/?igshid=123"), "GET", false));

  // Normal case with more than one domain
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL(),
                GURL("https://twitter.com/?ref_src=123"), "GET", false),
            GURL("https://twitter.com/"));
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL(),
                GURL("https://x.com/?ref_src=123"), "GET", false),
            GURL("https://x.com/"));
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL(),
                GURL("https://y.com/?ref_src=123"), "GET", false),
            GURL("https://y.com/"));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL(), GURL("https://z.com/?ref_src=123"),
      "GET", false));

  // Edge cases
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://example.com"), GURL(),
                GURL("https://brave.com/?sample1=123"), "GET", false),
            GURL("https://brave.com/"));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL(),
      GURL("https://example.com/?sample1=123"), "GET", false));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://example.com"), GURL(),
      GURL("https://brave.com/?sample2=123"), "GET", false));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://example.com"), GURL(),
      GURL("https://brave.com/?sample3=123"), "GET", false));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://example.com"), GURL(),
      GURL("https://brave.com/?sample4=123"), "GET", false));

  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL("https://google.com"), GURL(),
                GURL("https://mobile.facebook.com/?evil=666"), "GET", false),
            GURL("https://mobile.facebook.com/"));
  // theshining.com is part of the exclude list for the evil param. So, we
  // shouldn't be stripping it away.
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://google.com"), GURL(),
      GURL("https://theshining.com/?evil=123"), "GET", false));
}

TEST_F(BraveQueryFilter_ComponentEnabled, ConditionalFilteringTest) {
  // `ck_subscriber_id` param gets removed when `unsubscribe` not present in
  // url.
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?ck_subscriber_id=123"), "GET", false),
            GURL("https://test.com/"));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://unsubscribe.com/?ck_subscriber_id=123"), "GET", false));

  // `h_sid` param gets removed when /email/ not present in url.
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?h_sid=123"), "GET", false),
            GURL("https://test.com/"));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://test.com/email/?h_sid=123"), "GET", false));

  // `h_slt` param gets removed when /email/ not present in url.
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?h_slt=123"), "GET", false),
            GURL("https://test.com/"));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://test.com/email/?h_slt=123"), "GET", false));

  // `mkt_tok` param gets removed when `unsubscribe` or `emailWebView` not
  // present in url.
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?mkt_tok=123"), "GET", false),
            GURL("https://test.com/"));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://Unsubscribe.com/?mkt_tok=123"), "GET", false));
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://test.com/emailWebview?mkt_tok=123"), "GET", false));
}

TEST_F(BraveQueryFilter_ComponentEnabled, ConditionalFilteringWithoutAnyRules) {
  // We nuke the rules so we can test the hardcoded logic only.
  ResetRules();

  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?mkt_tok=123"), "GET", false),
            GURL("https://test.com/"));
}

class BraveQueryFilter_ComponentDisabled : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndDisableFeature(
        query_filter::features::kQueryFilterComponent);
    ASSERT_FALSE(instance());
  }

  query_filter::QueryFilterData* instance() const {
    return query_filter::QueryFilterData::GetInstance();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveQueryFilter_ComponentDisabled, NoStrippingOccurs) {
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://brave.com"),
      GURL("https://test.com/?gclid=123"), "GET", false));

  // Conditional filtering also fails.
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"), GURL("https://test.com/?mkt_tok=123"),
      "GET", false));
}
