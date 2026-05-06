// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/utils.h"

#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/query_filter/browser/query_filter_data.h"
#include "brave/components/query_filter/common/constants.h"
#include "brave/components/query_filter/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// Sample query filter JSON which would be written to a file during setup.
// This maps to the default rules which are hardcoded in utils.cc.
constexpr char kDefaultQueryFilterRules[] = R"json(
[
    {
        "include": [
            "*://*/*"
        ],
        "exclude": [],
        "params": [
            "__hsfp",
            "__hssc",
            "__hstc",
            "__s",
            "_bhlid",
            "_branch_match_id",
            "_branch_referrer",
            "_gl",
            "_hsenc",
            "_openstat",
            "at_recipient_id",
            "at_recipient_list",
            "bbeml",
            "bsft_clkid",
            "bsft_uid",
            "dclid",
            "et_rid",
            "fb_action_ids",
            "fb_comment_id",
            "fbclid",
            "gclid",
            "guce_referrer",
            "guce_referrer_sig",
            "hsCtaTracking",
            "irclickid",
            "mc_eid",
            "ml_subscriber",
            "ml_subscriber_hash",
            "msclkid",
            "mtm_cid",
            "oft_c",
            "oft_ck",
            "oft_d",
            "oft_id",
            "oft_ids",
            "oft_k",
            "oft_lk",
            "oft_sk",
            "oly_anon_id",
            "oly_enc_id",
            "pk_cid",
            "rb_clickid",
            "s_cid",
            "sc_customer",
            "sc_eh",
            "sc_uid",
            "sfmc_activityid",
            "sfmc_id",
            "sms_click",
            "sms_source",
            "sms_uph",
            "srsltid",
            "ss_email_id",
            "syclid",
            "ttclid",
            "twclid",
            "unicorn_click_id",
            "vero_conv",
            "vero_id",
            "vgo_ee",
            "wbraid",
            "wickedid",
            "yclid",
            "ymclid",
            "ysclid"
        ]
    },
    {
        "include": [
            "*://*.instagram.com/*",
            "*://instagram.com/*"
        ],
        "exclude": [],
        "params": [
            "igsh",
            "igshid"
        ]
    },
    {
        "include": [
            "*://*.twitter.com/*",
            "*://twitter.com/*",
            "*://*.x.com/*",
            "*://x.com/*"
        ],
        "exclude": [],
        "params": [
            "ref_src",
            "ref_url"
        ]
    },
    {
        "include": [
            "*://*.youtube.com/*",
            "*://youtube.com/*",
            "*://youtu.be/*"
        ],
        "exclude": [],
        "params": [
            "si"
        ]
    }
])json";

class BraveQueryFilter : public testing::Test,
                         public testing::WithParamInterface<bool> {
 public:
  void SetUp() override {
    if (GetParam()) {
      feature_list_.InitAndEnableFeature(
          query_filter::features::kQueryFilterComponent);
      // Populate the rules
      instance()->PopulateDataFromComponent(kDefaultQueryFilterRules);

    } else {
      feature_list_.InitAndDisableFeature(
          query_filter::features::kQueryFilterComponent);
    }
  }

  void TearDown() override {
    if (GetParam()) {
      instance()->ResetRulesForTesting();
      query_filter::SetScopedTrackerForTesting(nullptr);
    }
  }

  void SetScopedTrackerForTesting(
      query_filter::ScopedQueryTrackerType* trackers) {
    query_filter::SetScopedTrackerForTesting(trackers);
  }

 private:
  query_filter::QueryFilterData* instance() const {
    return query_filter::QueryFilterData::GetInstance();
  }

  base::test::ScopedFeatureList feature_list_;
};

INSTANTIATE_TEST_SUITE_P(All,
                         BraveQueryFilter,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "QueryFilterComponentEnabled"
                                             : "QueryFilterComponentDisabled";
                         });

TEST_P(BraveQueryFilter, FilterQueryTrackers) {
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

  // Conditional filtering, `ck_subscriber_id` param gets removed when
  // `unsubscribe` not present in url.
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?ck_subscriber_id=123"), "GET", false),
            GURL("https://test.com/"));
  // Conditional filtering, `h_sid` param gets removed when
  // /email/ not present in url.
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?h_sid=123"), "GET", false),
            GURL("https://test.com/"));
  // Conditional filtering, `h_slt` param gets removed when /email/
  // not present in url.
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?h_slt=123"), "GET", false),
            GURL("https://test.com/"));
  // Conditional filtering, `mkt_tok` param gets removed when `unsubscribe`
  // not present in url.
  EXPECT_EQ(query_filter::MaybeApplyQueryStringFilter(
                GURL(), GURL("https://brave.com"),
                GURL("https://test.com/?mkt_tok=123"), "GET", false),
            GURL("https://test.com/"));

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
  // Conditional filtering, test nothing filtered. `ck_subscriber_id` param with
  // `unsubscribe` present in url.
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://unsubscribe.com/?ck_subscriber_id=123"), "GET", false));
  // Conditional filtering, test nothing filtered. `h_sid` param with /email/
  // present in url.
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://test.com/email/?h_sid=123"), "GET", false));
  // Conditional filtering, test nothing filtered. `h_slt` param with `/email/`
  // present in url.
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://test.com/email/?h_slt=123"), "GET", false));
  // Conditional filtering, test nothing filtered. `mkt_tok` param with
  // `Unsubscribe` present in url.
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://Unsubscribe.com/?mkt_tok=123"), "GET", false));
  // Conditional filtering, test nothing filtered. `mkt_tok` param with
  // `emailWebview` present in url.
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL(), GURL("https://brave.com"),
      GURL("https://test.com/emailWebview?mkt_tok=123"), "GET", false));
}

TEST_F(BraveQueryFilter, ScopedQueryTrackingTest) {
  auto trackers = query_filter::ScopedQueryTrackerType(
      {{"igshid", {"instagram.com"}},
       {"ref_src", {"twitter.com", "x.com", "y.com"}},
       {"sample1", {"", " ", "brave.com", ""}},
       {"sample2", {" "}},
       {"sample3", {""}},
       {"sample4", {}},
       {"evil", {"*://*.facebook.com/*"}}});

  SetScopedTrackerForTesting(&trackers);

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

  // Wildcard case for the domain *://*.facebook.com/*.
  // TODO(https://github.com/brave/brave-browser/issues/55305): The new rules
  // would have URL wildcard support on domains. Currently we don't have that
  // support so query filter is not applied. When that's supported update this
  // test to EXPECT_EQ.
  EXPECT_FALSE(query_filter::MaybeApplyQueryStringFilter(
      GURL("https:/google.com"), GURL(),
      GURL("https://mobile.facebook.com/?evil=666"), "GET", false));
}

TEST_F(BraveQueryFilter, ConditionalFilteringTest) {
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
