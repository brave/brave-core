/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/get_signed_creds/get_signed_creds.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/test/mock_ledger_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetSignedCredsTest.*

using ::testing::_;
using ::testing::IsFalse;

namespace brave_rewards::internal::endpoint::promotion {

class GetSignedCredsTest : public MockLedgerTest {
 protected:
  GetSignedCreds creds_;
};

TEST_F(GetSignedCredsTest, ServerOK) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
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

  base::MockCallback<GetSignedCredsCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, mojom::CredsBatchPtr batch) {
        mojom::CredsBatch expected_batch;
        expected_batch.batch_proof = "zx0cdJhaB/OdYcUtnyXdi+lsoniN2KNgFU";
        expected_batch.public_key =
            "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";
        expected_batch.signed_creds =
            R"(["ijSZoLLG+EnRN916RUQcjiV6c4Wb6ItbnxXBFhz81EQ=","dj6glCJ2roHYcTFcXF21IrKx1uT/ptM7SJEdiEE1fG8=","nCF9a4KuASICVC0zrx2wGnllgIUxBMnylpu5SA+oBjI="])";  // NOLINT

        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_TRUE(expected_batch.Equals(*batch));
      });
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", "848d-995e186901a1",
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetSignedCredsTest, ServerError202) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 202;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetSignedCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::RETRY_SHORT, IsFalse())).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", "848d-995e186901a1",
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetSignedCredsTest, ServerError400) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetSignedCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::LEDGER_ERROR, IsFalse())).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", "848d-995e186901a1",
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetSignedCredsTest, ServerError404) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 404;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetSignedCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::NOT_FOUND, IsFalse())).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", "848d-995e186901a1",
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetSignedCredsTest, ServerError500) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetSignedCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::LEDGER_ERROR, IsFalse())).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", "848d-995e186901a1",
                 callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetSignedCredsTest, ServerErrorRandom) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetSignedCredsCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::LEDGER_ERROR, IsFalse())).Times(1);
  creds_.Request("ff50981d-47de-4210-848d-995e186901a1", "848d-995e186901a1",
                 callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace brave_rewards::internal::endpoint::promotion
