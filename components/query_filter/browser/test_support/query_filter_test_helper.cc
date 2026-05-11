// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/test_support/query_filter_test_helper.h"

#include "brave/components/query_filter/browser/query_filter_data.h"

namespace {
// Sample query filter JSON which would be written to a file during setup.
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
}  // namespace

namespace query_filter {
namespace test {

ScopedTestingQueryFilterRules::ScopedTestingQueryFilterRules() {
  QueryFilterData::GetInstance()->PopulateDataFromComponent(
      kDefaultQueryFilterRules);
}

bool ScopedTestingQueryFilterRules::UpdateRules(std::string_view rules_json) {
  QueryFilterData::GetInstance()->ResetRulesForTesting();
  return QueryFilterData::GetInstance()->PopulateDataFromComponent(rules_json);
}

ScopedTestingQueryFilterRules::~ScopedTestingQueryFilterRules() {
  QueryFilterData::GetInstance()->ResetRulesForTesting();
}

}  // namespace test

}  // namespace query_filter
