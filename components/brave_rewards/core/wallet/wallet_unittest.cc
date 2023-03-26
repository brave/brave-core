/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet/wallet.h"

#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/bat_ledger_test.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=*WalletTest*

namespace ledger {

class WalletTest : public BATLedgerTest {
 protected:
  mojom::CreateRewardsWalletResult CreateWalletIfNecessary() {
    auto* ledger = GetLedgerImpl();

    auto response = mojom::UrlResponse::New();
    response->status_code = net::HTTP_CREATED;
    response->body =
        "{\"paymentId\": \"37742974-3b80-461a-acfb-937e105e5af4\"}";

    AddNetworkResultForTesting(
        endpoint::promotion::GetServerUrl("/v3/wallet/brave"),
        mojom::UrlMethod::POST, std::move(response));

    base::RunLoop run_loop;
    mojom::CreateRewardsWalletResult result;
    ledger->wallet()->CreateWalletIfNecessary(
        absl::nullopt,
        base::BindLambdaForTesting(
            [&result, &run_loop](mojom::CreateRewardsWalletResult r) {
              result = r;
              run_loop.Quit();
            }));

    run_loop.Run();
    return result;
  }
};

TEST_F(WalletTest, GetWallet) {
  auto* ledger = GetLedgerImpl();
  bool corrupted = false;

  // When there is no current wallet information, `GetWallet` returns empty and
  // sets the corrupted flag to false.
  ledger->SetState(state::kWalletBrave, std::string());
  corrupted = true;
  mojom::RewardsWalletPtr wallet = ledger->wallet()->GetWallet(&corrupted);
  EXPECT_FALSE(wallet);
  EXPECT_FALSE(corrupted);

  // When there is invalid wallet information, `GetWallet` returns empty, sets
  // the corrupted flag to true, and does not modify prefs.
  ledger->SetState(state::kWalletBrave, std::string("BAD-DATA"));
  wallet = ledger->wallet()->GetWallet(&corrupted);
  EXPECT_FALSE(wallet);
  EXPECT_TRUE(corrupted);
  EXPECT_EQ(ledger->GetState<std::string>(state::kWalletBrave),
            "BAD-DATA");
}

TEST_F(WalletTest, CreateWallet) {
  auto* ledger = GetLedgerImpl();

  // Create a wallet when there is no current wallet information.
  ledger->SetState(state::kWalletBrave, std::string());
  mojom::CreateRewardsWalletResult result = CreateWalletIfNecessary();
  EXPECT_EQ(result, mojom::CreateRewardsWalletResult::kSuccess);
  mojom::RewardsWalletPtr wallet = ledger->wallet()->GetWallet();
  ASSERT_TRUE(wallet);
  EXPECT_TRUE(!wallet->payment_id.empty());
  EXPECT_TRUE(!wallet->recovery_seed.empty());

  // Create a wallet when there is corrupted wallet information.
  ledger->SetState(state::kWalletBrave, std::string("BAD-DATA"));
  result = CreateWalletIfNecessary();
  EXPECT_EQ(result, mojom::CreateRewardsWalletResult::kSuccess);
  wallet = ledger->wallet()->GetWallet();
  ASSERT_TRUE(wallet);
  EXPECT_TRUE(!wallet->payment_id.empty());
  EXPECT_TRUE(!wallet->recovery_seed.empty());
}

}  // namespace ledger
