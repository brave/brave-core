/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/endpoint/rewards/rewards_util.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/wallet_provider/solana/solana_wallet_provider.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=RewardsSolanaWalletProviderTest*

namespace brave_rewards::internal::wallet_provider {

class RewardsSolanaWalletProviderTest : public RewardsEngineTest {
 protected:
  void SetUp() override {
    std::string json = R"({
          "payment_id": "fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
          "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
        })";
    engine().SetState(state::kWalletBrave, std::move(json));
    InitializeEngine();
  }
};

TEST_F(RewardsSolanaWalletProviderTest, LinkingSuccess) {
  auto challenge_response = mojom::UrlResponse::New();
  challenge_response->status_code = net::HttpStatusCode::HTTP_CREATED;
  challenge_response->body =
      R"({"challengeId": "368d87a3-7749-4ebb-9f3a-2882c99078c7"})";

  AddNetworkResultForTesting(
      endpoint::promotion::GetServerUrl("/v3/wallet/challenges"),
      mojom::UrlMethod::POST, std::move(challenge_response));

  auto [login_params] =
      WaitFor<mojom::ExternalWalletLoginParamsPtr>([this](auto callback) {
        engine().Get<SolanaWalletProvider>().BeginLogin(std::move(callback));
      });

  ASSERT_TRUE(login_params);
  EXPECT_EQ(
      login_params->url,
      endpoint::rewards::GetServerUrl(
          "/connect/?msg=fa5dea51-6af4-44ca-801b-07b6df3dcfe4.368d87a3-7749-"
          "4ebb-9f3a-2882c99078c7&sig="
          "RDI71F4GBIcSMeURXWpX3c9BIU5Uq1GEJn5bpWed8k9bcOqSuJFrShJwndGgBf-"
          "kfPLoXM0T62VmGhhaY632DQ%3D%3D"));
  EXPECT_EQ(login_params->cookies["__Secure-CSRF_TOKEN"],
            "368d87a3-7749-4ebb-9f3a-2882c99078c7");

  auto get_wallet_response = mojom::UrlResponse::New();
  get_wallet_response->status_code = net::HttpStatusCode::HTTP_OK;
  get_wallet_response->body =
      R"({
          "depositAccountProvider": {
            "name": "solana",
            "id": "4668ba96-7129-5e85-abdc-0c144ab7883c",
            "linkingId": "4668ba96-7129-5e85-abdc-0c144ab78834"
          }
      })";

  AddNetworkResultForTesting(
      endpoint::promotion::GetServerUrl(
          "/v4/wallets/fa5dea51-6af4-44ca-801b-07b6df3dcfe4"),
      mojom::UrlMethod::GET, std::move(get_wallet_response));

  engine().Get<SolanaWalletProvider>().PollWalletStatus();
  task_environment().RunUntilIdle();

  auto [external_wallet] =
      WaitFor<mojom::ExternalWalletPtr>([this](auto callback) {
        engine().GetExternalWallet(std::move(callback));
      });

  ASSERT_TRUE(external_wallet);
  EXPECT_EQ(external_wallet->status, mojom::WalletStatus::kConnected);
  EXPECT_EQ(external_wallet->type, "solana");
  EXPECT_EQ(external_wallet->address, "4668ba96-7129-5e85-abdc-0c144ab7883c");

  auto solana_balance = mojom::SolanaAccountBalance::New();
  solana_balance->amount = "1234";
  solana_balance->decimals = 2;

  engine_client().AddSPLAccountBalanceResultForTesting(
      "4668ba96-7129-5e85-abdc-0c144ab7883c",
      "EPeUFDgHRxs9xxEPVaL6kfGQvCon7jmAWKVUHuux1Tpz",
      std::move(solana_balance));

  auto [balance_result, balance_value] =
      WaitFor<mojom::Result, double>([this](auto callback) {
        engine().Get<SolanaWalletProvider>().FetchBalance(std::move(callback));
      });

  EXPECT_EQ(balance_result, mojom::Result::OK);
  EXPECT_EQ(balance_value, 12.34);
}

}  // namespace brave_rewards::internal::wallet_provider
