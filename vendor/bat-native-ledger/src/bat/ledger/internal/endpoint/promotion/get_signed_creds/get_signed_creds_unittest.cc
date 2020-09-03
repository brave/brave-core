/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_signed_creds/get_signed_creds.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetSignedCredsTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class GetSignedCredsTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetSignedCreds> creds_;

  GetSignedCredsTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    creds_ = std::make_unique<GetSignedCreds>(mock_ledger_impl_.get());
  }
};

TEST_F(GetSignedCredsTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
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
            callback(response);
          }));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      "848d-995e186901a1",
      [](const type::Result result, type::CredsBatchPtr batch) {
        type::CredsBatch expected_batch;
        expected_batch.batch_proof = "zx0cdJhaB/OdYcUtnyXdi+lsoniN2KNgFU";
        expected_batch.public_key =
            "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";
        expected_batch.signed_creds =
            R"(["ijSZoLLG+EnRN916RUQcjiV6c4Wb6ItbnxXBFhz81EQ=","dj6glCJ2roHYcTFcXF21IrKx1uT/ptM7SJEdiEE1fG8=","nCF9a4KuASICVC0zrx2wGnllgIUxBMnylpu5SA+oBjI="])"; // NOLINT

        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_TRUE(expected_batch.Equals(*batch));
      });
}

TEST_F(GetSignedCredsTest, ServerError202) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 202;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      "848d-995e186901a1",
      [](const type::Result result, type::CredsBatchPtr batch) {
        EXPECT_EQ(result, type::Result::RETRY_SHORT);
        EXPECT_TRUE(!batch);
      });
}

TEST_F(GetSignedCredsTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      "848d-995e186901a1",
      [](const type::Result result, type::CredsBatchPtr batch) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(!batch);
      });
}

TEST_F(GetSignedCredsTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 404;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      "848d-995e186901a1",
      [](const type::Result result, type::CredsBatchPtr batch) {
        EXPECT_EQ(result, type::Result::NOT_FOUND);
        EXPECT_TRUE(!batch);
      });
}

TEST_F(GetSignedCredsTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      "848d-995e186901a1",
      [](const type::Result result, type::CredsBatchPtr batch) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(!batch);
      });
}

TEST_F(GetSignedCredsTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      "848d-995e186901a1",
      [](const type::Result result, type::CredsBatchPtr batch) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(!batch);
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
