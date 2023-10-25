/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/payment/get_credentials/get_credentials.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetCredentialsTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

class GetCredentialsTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  GetCredentials creds_{mock_engine_impl_};
};

TEST_F(GetCredentialsTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCredentialsCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, mojom::CredsBatchPtr batch) {
        mojom::CredsBatch expected_batch;
        expected_batch.batch_proof = "zx0cdJhaB/OdYcUtnyXdi+lsoniN2KNgFU";
        expected_batch.public_key =
            "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";
        expected_batch.signed_creds =
            R"(["ijSZoLLG+EnRN916RUQcjiV6c4Wb6ItbnxXBFhz81EQ=","dj6glCJ2roHYcTFcXF21IrKx1uT/ptM7SJEdiEE1fG8=","nCF9a4KuASICVC0zrx2wGnllgIUxBMnylpu5SA+oBjI="])";  // NOLINT

        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_TRUE(expected_batch.Equals(*batch));
      });
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCredentialsTest, ServerError202) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 202;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::RETRY_SHORT, _)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCredentialsTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::RETRY, _)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCredentialsTest, ServerError404) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 404;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::RETRY, _)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCredentialsTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::RETRY, _)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCredentialsTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCredentialsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::RETRY, _)).Times(1);
  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
