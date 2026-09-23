/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// clang-format off
// npm run test -- brave_unit_tests --filter="*RewardsProtocolNavigationThrottleTest*"
// clang-format on

using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards {

GURL TransformUrl(const GURL& url);

bool IsValidWalletProviderRedirect(
    const GURL& referrer_url,
    const GURL& redirect_url,
    const std::map<std::string, std::vector<GURL>>& allowed_referrer_urls);

// clang-format off
using RewardsProtocolNavigationThrottleTestParamType = std::tuple<
    std::string,  // test name suffix
    std::string,  // referrer URL
    std::string,  // redirect URL
    bool          // expected result
>;
// clang-format on

class RewardsProtocolNavigationThrottleTest
    : public TestWithParam<RewardsProtocolNavigationThrottleTestParamType> {};

TEST(TransformUrl, RewardsProtocolNavigationThrottleTest) {
  EXPECT_EQ(TransformUrl(GURL("rewards://uphold/"
                              "authorization")),
            GURL("chrome://rewards/uphold/authorization"));

  EXPECT_EQ(
      TransformUrl(GURL(
          "rewards://bitflyer/"
          "authorization?error=access_denied&state="
          "2B57A19DD21010147403EC0FFDD4C4F0634DAA6094A98AB5A0F2D9A67C3E16F6&"
          "error_description=User+does+not+meet+minimum+requirements")),
      GURL("chrome://rewards/bitflyer/authorization?error=access_denied&state="
           "2B57A19DD21010147403EC0FFDD4C4F0634DAA6094A98AB5A0F2D9A67C3E16F6&"
           "error_description=User+does+not+meet+minimum+requirements"));
}

TEST_P(RewardsProtocolNavigationThrottleTest, Paths) {
  const auto& [ignore, referrer_url, redirect_url, result] = GetParam();

  const std::map<std::string, std::vector<GURL>> allowed_referrer_urls{
      {"bitflyer", {GURL("https://bitflyer.com")}},
      {"uphold", {GURL("https://uphold.com")}}};

  const auto transformed_url = TransformUrl(GURL(redirect_url));

  EXPECT_EQ(IsValidWalletProviderRedirect(GURL(referrer_url), transformed_url,
                                          allowed_referrer_urls),
            result);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  IsValidWalletProviderRedirect,
  RewardsProtocolNavigationThrottleTest,
  Values(
    RewardsProtocolNavigationThrottleTestParamType{
      "no_redirect_to_enable",
      "https://bitflyer.com",
      "rewards://enable",
      false
    },
    RewardsProtocolNavigationThrottleTestParamType{
      "no_redirect_for_unknown_wallet_providers",
      "https://unknown.com",
      "rewards://unknown/authorization",
      false
    },
    RewardsProtocolNavigationThrottleTestParamType{
      "no_redirect_to_each_others_redirect_url",
      "https://bitflyer.com",
      "rewards://uphold/authorization",
      false
    },
    RewardsProtocolNavigationThrottleTestParamType{
      "success",
      "https://uphold.com",
      "rewards://uphold/authorization",
      true
    }),
  [](const auto& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards
