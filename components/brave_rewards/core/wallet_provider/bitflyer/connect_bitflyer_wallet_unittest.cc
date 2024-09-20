/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/connect_bitflyer_wallet.h"

#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal::bitflyer {

class RewardsConnectBitflyerWalletTest : public RewardsEngineTest {};

TEST_F(RewardsConnectBitflyerWalletTest, LoginURL) {
  auto& config = engine().Get<EnvironmentConfig>();
  ConnectBitFlyerWallet connect(engine());

  auto actual = connect.GenerateLoginURL();
  auto& oauth_info = connect.GetOAuthStateForTesting();
  auto challenge = util::GeneratePKCECodeChallenge(oauth_info.code_verifier);

  auto expected_url = config.bitflyer_url().Resolve(
      base::StrCat({"/ex/OAuth/authorize"
                    "?client_id=",
                    config.bitflyer_client_id(),
                    "&scope=assets+create_deposit_id+withdraw_to_deposit_id"
                    "&redirect_uri=rewards%3A%2F%2Fbitflyer%2Fauthorization"
                    "&state=",
                    oauth_info.one_time_string,
                    "&response_type=code"
                    "&code_challenge_method=S256"
                    "&code_challenge=",
                    challenge}));

  EXPECT_EQ(actual, expected_url.spec());
}

}  // namespace brave_rewards::internal::bitflyer
