/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet/wallet.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {

class RewardsWalletTest : public RewardsEngineTest {
 protected:
  mojom::CreateRewardsWalletResult CreateWalletIfNecessary() {
    auto response = mojom::UrlResponse::New();
    response->status_code = net::HTTP_CREATED;
    response->body =
        "{\"paymentId\": \"37742974-3b80-461a-acfb-937e105e5af4\"}";

    client().AddNetworkResultForTesting(engine()
                                            .Get<EnvironmentConfig>()
                                            .rewards_grant_url()
                                            .Resolve("/v3/wallet/brave")
                                            .spec(),
                                        mojom::UrlMethod::POST,
                                        std::move(response));

    base::RunLoop run_loop;
    mojom::CreateRewardsWalletResult result;
    engine().wallet()->CreateWalletIfNecessary(
        std::nullopt,
        base::BindLambdaForTesting(
            [&result, &run_loop](mojom::CreateRewardsWalletResult r) {
              result = r;
              run_loop.Quit();
            }));

    run_loop.Run();
    return result;
  }
};

TEST_F(RewardsWalletTest, GetWallet) {
  bool corrupted = false;

  // When there is no current wallet information, `GetWallet` returns empty and
  // sets the corrupted flag to false.
  engine().Get<Prefs>().SetString(prefs::kWalletBrave, "");
  corrupted = true;
  mojom::RewardsWalletPtr wallet = engine().wallet()->GetWallet(&corrupted);
  EXPECT_FALSE(wallet);
  EXPECT_FALSE(corrupted);

  // When there is invalid wallet information, `GetWallet` returns empty, sets
  // the corrupted flag to true, and does not modify prefs.
  engine().Get<Prefs>().SetString(prefs::kWalletBrave, "BAD-DATA");
  wallet = engine().wallet()->GetWallet(&corrupted);
  EXPECT_FALSE(wallet);
  EXPECT_TRUE(corrupted);
  EXPECT_EQ(engine().Get<Prefs>().GetString(prefs::kWalletBrave), "BAD-DATA");
}

TEST_F(RewardsWalletTest, CreateWallet) {
  // Create a wallet when there is no current wallet information.
  engine().Get<Prefs>().SetString(prefs::kWalletBrave, "");
  mojom::CreateRewardsWalletResult result = CreateWalletIfNecessary();
  EXPECT_EQ(result, mojom::CreateRewardsWalletResult::kSuccess);
  mojom::RewardsWalletPtr wallet = engine().wallet()->GetWallet();
  ASSERT_TRUE(wallet);
  EXPECT_TRUE(!wallet->payment_id.empty());
  EXPECT_TRUE(!wallet->recovery_seed.empty());

  // Create a wallet when there is corrupted wallet information.
  engine().Get<Prefs>().SetString(prefs::kWalletBrave, "BAD-DATA");
  result = CreateWalletIfNecessary();
  EXPECT_EQ(result, mojom::CreateRewardsWalletResult::kSuccess);
  wallet = engine().wallet()->GetWallet();
  ASSERT_TRUE(wallet);
  EXPECT_TRUE(!wallet->payment_id.empty());
  EXPECT_TRUE(!wallet->recovery_seed.empty());
}

}  // namespace brave_rewards::internal
