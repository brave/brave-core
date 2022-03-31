/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_suggestions_claim/post_suggestions_claim.h"

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

// npm run test -- brave_unit_tests --filter=PostSuggestionsClaimTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostSuggestionsClaimTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostSuggestionsClaim> claim_;
  std::unique_ptr<credential::CredentialsRedeem> redeem_;

  PostSuggestionsClaimTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    claim_ = std::make_unique<PostSuggestionsClaim>(mock_ledger_impl_.get());

    type::UnblindedToken token;
    token.token_value =
        "s1OrSZUvo/33u3Y866mQaG/"
        "b6d94TqMThLal4+DSX4UrR4jT+GtTErim+"
        "FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R"
        "6";  // NOLINT
    token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";
    redeem_ = std::make_unique<credential::CredentialsRedeem>();
    redeem_->publisher_key = "brave.com";
    redeem_->type = type::RewardsType::ONE_TIME_TIP;
    redeem_->processor = type::ContributionProcessor::BRAVE_TOKENS;
    redeem_->token_list = {token};
    redeem_->order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
    redeem_->contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";
  }

  void SetUp() override {
    const std::string wallet = R"({
      "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
      "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
    })";
    ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBrave))
        .WillByDefault(testing::Return(wallet));
  }
};

TEST_F(PostSuggestionsClaimTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"(
              {"drainId": "1af0bf71-c81c-4b18-9188-a0d3c4a1b53b"}
            )";
            callback(response);
          }));

  claim_->Request(*redeem_,
                  [](const type::Result result, std::string drain_id) {
                    EXPECT_EQ(result, type::Result::LEDGER_OK);
                    EXPECT_EQ(drain_id, "1af0bf71-c81c-4b18-9188-a0d3c4a1b53b");
                  });
}

TEST_F(PostSuggestionsClaimTest, ServerNeedsRetry) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(*redeem_,
                  [](const type::Result result, std::string drain_id) {
                    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                    EXPECT_EQ(drain_id, "");
                  });
}

TEST_F(PostSuggestionsClaimTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(*redeem_,
                  [](const type::Result result, std::string drain_id) {
                    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                    EXPECT_EQ(drain_id, "");
                  });
}

TEST_F(PostSuggestionsClaimTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(*redeem_,
                  [](const type::Result result, std::string drain_id) {
                    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                    EXPECT_EQ(drain_id, "");
                  });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
