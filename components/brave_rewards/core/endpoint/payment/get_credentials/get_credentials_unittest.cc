/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/get_credentials/get_credentials.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsGetCredentialsTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().rewards_payment_url().Resolve(
            "/v1/orders/pl2okf23-f2f02kf2fm2-msdkfsodkfds"
            "/credentials/ff50981d-47de-4210-848d-995e186901a1");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::GET, std::move(response));

    endpoint::payment::GetCredentials endpoint(engine());

    return WaitForValues<mojom::Result, mojom::CredsBatchPtr>(
        [&](auto callback) {
          endpoint.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                           "ff50981d-47de-4210-848d-995e186901a1",
                           std::move(callback));
        });
  }
};

TEST_F(RewardsGetCredentialsTest, ServerOK) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"({
        "id": "9c9aed7f-b349-452e-80a8-95faf2b1600d",
        "orderId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
        "issuerId": "138bf9ca-69fe-4540-9ac4-bc65baddc4a0",
        "signedCreds": [
          "ijSZoLLG+EnRN916RUQcjiV6c4Wb6ItbnxXBFhz81EQ=",
          "dj6glCJ2roHYcTFcXF21IrKx1uT/ptM7SJEdiEE1fG8=",
          "nCF9a4KuASICVC0zrx2wGnllgIUxBMnylpu5SA+oBjI="
        ],
        "batchProof": "zx0cdJhaB/OdYcUtnyXdi+lsoniN2KNgFU",
        "publicKey": "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I="
      })";

  auto [result, batch] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(batch->batch_proof, "zx0cdJhaB/OdYcUtnyXdi+lsoniN2KNgFU");
  EXPECT_EQ(batch->public_key, "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=");
  EXPECT_EQ(batch->signed_creds,
            R"(["ijSZoLLG+EnRN916RUQcjiV6c4Wb6ItbnxXBFhz81EQ=",)"
            R"("dj6glCJ2roHYcTFcXF21IrKx1uT/ptM7SJEdiEE1fG8=",)"
            R"("nCF9a4KuASICVC0zrx2wGnllgIUxBMnylpu5SA+oBjI="])");
}

TEST_F(RewardsGetCredentialsTest, ServerError202) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 202;

  auto [result, batch] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::RETRY_SHORT);
  EXPECT_FALSE(batch);
}

TEST_F(RewardsGetCredentialsTest, ServerError400) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 400;

  auto [result, batch] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::RETRY);
  EXPECT_FALSE(batch);
}

TEST_F(RewardsGetCredentialsTest, ServerError404) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 404;

  auto [result, batch] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::RETRY);
  EXPECT_FALSE(batch);
}

TEST_F(RewardsGetCredentialsTest, ServerError500) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 500;

  auto [result, batch] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::RETRY);
  EXPECT_FALSE(batch);
}

TEST_F(RewardsGetCredentialsTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;

  auto [result, batch] = Request(std::move(response));

  EXPECT_EQ(result, mojom::Result::RETRY);
  EXPECT_FALSE(batch);
}

}  // namespace brave_rewards::internal
