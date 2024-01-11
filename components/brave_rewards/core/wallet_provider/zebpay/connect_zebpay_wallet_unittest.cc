/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/zebpay/connect_zebpay_wallet.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal::zebpay {

class RewardsConnectZebPayWalletTest : public RewardsEngineTest {};

TEST_F(RewardsConnectZebPayWalletTest, LoginURL) {
  auto& config = engine().Get<EnvironmentConfig>();
  ConnectZebPayWallet connect(engine());

  auto actual = connect.GenerateLoginURL();

  auto expected_url = URLHelpers::Resolve(
      config.zebpay_oauth_url(),
      {"/account/login?returnUrl="
       "%2Fconnect%2Fauthorize%2Fcallback%3F"
       "client_id%3D60189f6b-27e2-4652-9a48-14b3356b1182%26"
       "grant_type%3Dauthorization_code%26"
       "redirect_uri%3Drewards%253A%252F%252Fzebpay%252Fauthorization%26"
       "response_type%3Dcode%26"
       "scope%3Dopenid%2Bprofile%26"
       "state%3D",
       connect.GetOAuthStateForTesting().one_time_string});

  EXPECT_EQ(actual, expected_url.spec());
}

}  // namespace brave_rewards::internal::zebpay
