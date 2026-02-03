/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/wallet_provider/gemini/connect_gemini_wallet.h"

#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/engine/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"

namespace brave_rewards::internal::gemini {

class RewardsConnectGeminiWalletTest : public RewardsEngineTest {};

TEST_F(RewardsConnectGeminiWalletTest, LoginURL) {
  auto& config = engine().Get<EnvironmentConfig>();
  ConnectGeminiWallet connect(engine());

  auto actual = connect.GenerateLoginURL();

  auto expected_url = config.gemini_oauth_url().Resolve(
      base::StrCat({"auth"
                    "?scope=balances%3Aread%2Chistory%3Aread%2Ccrypto%3Asend%2C"
                    "account%3Aread%2Cpayments%3Acreate%2Cpayments%3Asend%2C"
                    "&state=",
                    connect.GetOAuthStateForTesting().one_time_string,
                    "&response_type=code"}));
  EXPECT_EQ(actual, expected_url.spec());
}

}  // namespace brave_rewards::internal::gemini
