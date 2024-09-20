/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/gemini/connect_gemini_wallet.h"

#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal::gemini {

class RewardsConnectGeminiWalletTest : public RewardsEngineTest {};

TEST_F(RewardsConnectGeminiWalletTest, LoginURL) {
  auto& config = engine().Get<EnvironmentConfig>();
  ConnectGeminiWallet connect(engine());

  auto actual = connect.GenerateLoginURL();

  auto expected_url = config.gemini_oauth_url().Resolve(
      base::StrCat({"/auth?client_id=", config.gemini_client_id(),
                    "&scope=balances%3Aread%2Chistory%3Aread%2Ccrypto%3Asend%2C"
                    "account%3Aread%2Cpayments%3Acreate%2Cpayments%3Asend%2C"
                    "&redirect_uri=rewards%3A%2F%2Fgemini%2Fauthorization"
                    "&state=",
                    connect.GetOAuthStateForTesting().one_time_string,
                    "&response_type=code"}));
  EXPECT_EQ(actual, expected_url.spec());
}

}  // namespace brave_rewards::internal::gemini
