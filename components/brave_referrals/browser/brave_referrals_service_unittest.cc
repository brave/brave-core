/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"
#include "brave/common/network_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

typedef testing::Test BraveReferralsServiceTest;

TEST(BraveReferralsServiceTest, GetMatchingReferralHeaders) {
  // format matching https://laptop-updates.brave.com/promo/custom-headers
  std::string brave_referral_headers =
    R"([
    {
      "domains": [
        "brave.com",
        "subdomain.brave.com"
      ],
      "headers": {
        "X-Brave-Partner": "brave"
      },
      "cookieNames": [],
      "expiration": 31536000000
    }
  ])";

  // load JSON similar to BraveReferralsService::OnReferralHeadersLoadComplete
  base::Optional<base::Value> root =
      base::JSONReader().ReadToValue(brave_referral_headers);
  ASSERT_TRUE(root);
  ASSERT_TRUE(root->is_list());

  TestingPrefServiceSimple local_state;

  local_state.registry()->RegisterListPref(
      kReferralHeaders, root.value().Clone());

  // get value similar to BraveRequestHandler::OnReferralHeadersChanged
  const base::ListValue* referral_headers =
          local_state.GetList(kReferralHeaders);

  const base::DictionaryValue* request_headers_dict = nullptr;
  GURL request_url("https://subdomain.brave.com/api/v1/controller_name");

  EXPECT_TRUE(brave::BraveReferralsService::GetMatchingReferralHeaders(
    *referral_headers,
    &request_headers_dict,
    request_url));

  bool has_partner_header = false;
  for (const auto& it : request_headers_dict->DictItems()) {
    if (it.first == kBravePartnerHeader) {
      has_partner_header = true;
    }
  }
  EXPECT_TRUE(has_partner_header);
}
