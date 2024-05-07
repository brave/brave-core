/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/linkage_checker.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {

class RewardsLinkageChecker : public RewardsEngineTest {
 protected:
  void SetUp() override {
    engine().Get<Prefs>().SetString(prefs::kWalletBrave,
                                    R"({
          "payment_id": "fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
          "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
        })");
  }

  void SetExternalWalletData() {
    engine().Get<Prefs>().SetString(prefs::kExternalWalletType, "uphold");

    engine().Get<Prefs>().SetString(prefs::kWalletUphold,
                                    FakeEncryption::Base64EncryptString(
                                        R"({
              "token": "sI5rKiy6ijzbbJgE2MMFzAbTc6udYYXEi3wzS9iknP6n",
              "address": "6a752063-8958-44d5-b5db-71543f18567d",
              "status": 2,
              "user_name": "random_user",
              "account_url": "https://random.domain/account",
              "fees": {}
            })"));
  }

  void AddEndpointResponse(const std::string& body) {
    auto response = mojom::UrlResponse::New();
    response->status_code = net::HTTP_OK;
    response->body = body;

    client().AddNetworkResultForTesting(
        engine()
            .Get<EnvironmentConfig>()
            .rewards_grant_url()
            .Resolve("/v4/wallets/fa5dea51-6af4-44ca-801b-07b6df3dcfe4")
            .spec(),
        mojom::UrlMethod::GET, std::move(response));
  }
};

TEST_F(RewardsLinkageChecker, ServerLinked) {
  SetExternalWalletData();

  AddEndpointResponse(
      R"({
        "depositAccountProvider": {
        "id": "2d7519f4-cb7b-41b7-9f33-9d716f2e7915",
        "linkingId": "2698ba94-7129-5a85-abcd-0c166ab75189",
        "name": "uphold"
      })");

  InitializeEngine();
  task_environment().RunUntilIdle();

  auto external_wallet =
      WaitFor<mojom::ExternalWalletPtr>([this](auto callback) {
        engine().GetExternalWallet(std::move(callback));
      });

  ASSERT_TRUE(external_wallet);
  EXPECT_EQ(external_wallet->status, mojom::WalletStatus::kConnected);
}

TEST_F(RewardsLinkageChecker, ServerUnlinked) {
  SetExternalWalletData();

  AddEndpointResponse(
      R"({
        "depositAccountProvider": {
          "id": "2d7519f4-cb7b-41b7-9f33-9d716f2e7915",
          "linkingId": "",
          "name": "uphold"
        }
      })");

  InitializeEngine();
  task_environment().RunUntilIdle();

  auto external_wallet =
      WaitFor<mojom::ExternalWalletPtr>([this](auto callback) {
        engine().GetExternalWallet(std::move(callback));
      });

  EXPECT_FALSE(external_wallet);
}

}  // namespace brave_rewards::internal
