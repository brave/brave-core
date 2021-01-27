/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/bitflyer/post_claim/post_claim_bitflyer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostClaimBitflyerTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace bitflyer {

class PostClaimBitflyerTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostClaimBitflyer> claim_;

  PostClaimBitflyerTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    claim_ = std::make_unique<PostClaimBitflyer>(mock_ledger_impl_.get());
  }

  void SetUp() override {
    const std::string wallet = R"({
      "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
      "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
    })";
    ON_CALL(*mock_ledger_client_, GetEncryptedStringState(state::kWalletBrave))
        .WillByDefault(testing::Return(wallet));
  }
};

TEST_F(PostClaimBitflyerTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request("83b3b77b-e7c3-455b-adda-e476fa0656d2",
                  [](const type::Result result) {
                    EXPECT_EQ(result, type::Result::LEDGER_OK);
                  });
}

TEST_F(PostClaimBitflyerTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request("83b3b77b-e7c3-455b-adda-e476fa0656d2",
                  [](const type::Result result) {
                    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                  });
}

TEST_F(PostClaimBitflyerTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 404;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request("83b3b77b-e7c3-455b-adda-e476fa0656d2",
                  [](const type::Result result) {
                    EXPECT_EQ(result, type::Result::NOT_FOUND);
                  });
}

TEST_F(PostClaimBitflyerTest, ServerError409) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 409;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request("83b3b77b-e7c3-455b-adda-e476fa0656d2",
                  [](const type::Result result) {
                    EXPECT_EQ(result, type::Result::ALREADY_EXISTS);
                  });
}

TEST_F(PostClaimBitflyerTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request("83b3b77b-e7c3-455b-adda-e476fa0656d2",
                  [](const type::Result result) {
                    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                  });
}

TEST_F(PostClaimBitflyerTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request("83b3b77b-e7c3-455b-adda-e476fa0656d2",
                  [](const type::Result result) {
                    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                  });
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
