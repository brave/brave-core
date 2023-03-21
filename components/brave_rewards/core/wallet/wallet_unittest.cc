/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=WalletTest*

using ::testing::_;

namespace ledger {

class WalletTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  wallet::Wallet wallet_{&mock_ledger_impl_};
};

TEST_F(WalletTest, GetWallet) {
  EXPECT_CALL(*mock_ledger_impl_.mock_client(),
              GetStringState(state::kWalletBrave, _))
      .Times(2)
      // When there is no current wallet information, `GetWallet` returns empty
      // and sets the corrupted flag to false.
      .WillOnce([](const std::string&, auto callback) {
        std::move(callback).Run(std::move(""));
      })
      // When there is invalid wallet information, `GetWallet` returns empty
      // and sets the corrupted flag to true.
      .WillOnce([](const std::string&, auto callback) {
        std::move(callback).Run(std::move("BAD-DATA"));
      });

  bool corrupted = true;
  auto wallet = wallet_.GetWallet(&corrupted);
  EXPECT_FALSE(wallet);
  EXPECT_FALSE(corrupted);

  wallet = wallet_.GetWallet(&corrupted);
  EXPECT_FALSE(wallet);
  EXPECT_TRUE(corrupted);
}

TEST_F(WalletTest, CreateWallet) {
  std::string wallet_state;

  ON_CALL(*mock_ledger_impl_.mock_client(),
          SetStringState(state::kWalletBrave, _, _))
      .WillByDefault(
          [&](const std::string&, const std::string& value, auto callback) {
            wallet_state = value;
            std::move(callback).Run();
          });

  ON_CALL(*mock_ledger_impl_.mock_client(),
          GetStringState(state::kWalletBrave, _))
      .WillByDefault([&](const std::string&, auto callback) {
        std::move(callback).Run(wallet_state);
      });

  EXPECT_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .Times(2)
      .WillRepeatedly([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_CREATED;
        response->body = R"(
          {
            "paymentId": "37742974-3b80-461a-acfb-937e105e5af4"
          }
        )";
        std::move(callback).Run(std::move(response));
      });

  // Create a wallet when there is no current wallet information.
  wallet_.CreateWalletIfNecessary(
      absl::nullopt,
      base::BindLambdaForTesting([](mojom::CreateRewardsWalletResult result) {
        EXPECT_EQ(result, mojom::CreateRewardsWalletResult::kSuccess);
      }));

  task_environment_.RunUntilIdle();

  auto wallet = wallet_.GetWallet();
  ASSERT_TRUE(wallet);
  EXPECT_EQ(wallet->payment_id, "37742974-3b80-461a-acfb-937e105e5af4");
  EXPECT_TRUE(!wallet->recovery_seed.empty());

  // Create a wallet when there is corrupted wallet information.
  wallet_state = "BAD-DATA";
  wallet_.CreateWalletIfNecessary(
      absl::nullopt,
      base::BindLambdaForTesting([](mojom::CreateRewardsWalletResult result) {
        EXPECT_EQ(result, mojom::CreateRewardsWalletResult::kSuccess);
      }));

  task_environment_.RunUntilIdle();

  wallet = wallet_.GetWallet();
  ASSERT_TRUE(wallet);
  EXPECT_EQ(wallet->payment_id, "37742974-3b80-461a-acfb-937e105e5af4");
  EXPECT_TRUE(!wallet->recovery_seed.empty());
}

}  // namespace ledger
