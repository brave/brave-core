/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/wallet_provider/bitflyer/connect_bitflyer_wallet.h"

#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/engine/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"
#include "brave/components/brave_rewards/core/engine/util/random_util.h"

namespace brave_rewards::internal::bitflyer {

class RewardsConnectBitflyerWalletTest : public RewardsEngineTest {};

TEST_F(RewardsConnectBitflyerWalletTest, LoginURL) {
  auto& config = engine().Get<EnvironmentConfig>();
  ConnectBitFlyerWallet connect(engine());

  auto actual = connect.GenerateLoginURL();
  auto& oauth_info = connect.GetOAuthStateForTesting();
  auto challenge = util::GeneratePKCECodeChallenge(oauth_info.code_verifier);

  auto expected_url =
      config.gate3_oauth_url("bitflyer")
          .Resolve(base::StrCat(
              {"auth"
               "?scope=assets+create_deposit_id+withdraw_to_deposit_id"
               "&state=",
               oauth_info.one_time_string,
               "&response_type=code"
               "&code_challenge_method=S256"
               "&code_challenge=",
               challenge}));

  EXPECT_EQ(actual, expected_url.spec());
}

}  // namespace brave_rewards::internal::bitflyer
