/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/uphold/connect_uphold_wallet.h"

#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal::uphold {

class RewardsConnectUpholdWalletTest : public RewardsEngineTest {};

TEST_F(RewardsConnectUpholdWalletTest, LoginURL) {
  auto& config = engine().Get<EnvironmentConfig>();
  ConnectUpholdWallet connect(engine());

  auto actual = connect.GenerateLoginURL();

  auto expected_url = config.uphold_oauth_url().Resolve(base::StrCat(
      {"/authorize/", config.uphold_client_id(),
       "?scope=cards%3Aread+cards%3Awrite+user%3Aread+transactions%3Aread+"
       "transactions%3Atransfer%3Aapplication+transactions%3Atransfer%3Aothers"
       "&intention=login"
       "&state=",
       connect.GetOAuthStateForTesting().one_time_string}));

  EXPECT_EQ(actual, expected_url.spec());
}

}  // namespace brave_rewards::internal::uphold
