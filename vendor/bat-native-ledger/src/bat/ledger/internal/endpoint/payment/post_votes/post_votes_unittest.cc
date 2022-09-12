/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/payment/post_votes/post_votes.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostVotesTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace payment {

class PostVotesTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostVotes> votes_;

  PostVotesTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    votes_ = std::make_unique<PostVotes>(mock_ledger_impl_.get());
  }
};

TEST_F(PostVotesTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  mojom::UnblindedToken token;
  token.token_value = "s1OrSZUvo/33u3Y866mQaG/b6d94TqMThLal4+DSX4UrR4jT+GtTErim+FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R6";  // NOLINT
  token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";

  credential::CredentialsRedeem redeem;
  redeem.publisher_key = "brave.com";
  redeem.type = mojom::RewardsType::ONE_TIME_TIP;
  redeem.processor = mojom::ContributionProcessor::UPHOLD;
  redeem.token_list = {token};
  redeem.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
  redeem.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";

  votes_->Request(redeem, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_OK);
  });
}

TEST_F(PostVotesTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  mojom::UnblindedToken token;
  token.token_value = "s1OrSZUvo/33u3Y866mQaG/b6d94TqMThLal4+DSX4UrR4jT+GtTErim+FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R6";  // NOLINT
  token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";

  credential::CredentialsRedeem redeem;
  redeem.publisher_key = "brave.com";
  redeem.type = mojom::RewardsType::ONE_TIME_TIP;
  redeem.processor = mojom::ContributionProcessor::UPHOLD;
  redeem.token_list = {token};
  redeem.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
  redeem.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";

  votes_->Request(redeem, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::RETRY_SHORT);
  });
}

TEST_F(PostVotesTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  mojom::UnblindedToken token;
  token.token_value = "s1OrSZUvo/33u3Y866mQaG/b6d94TqMThLal4+DSX4UrR4jT+GtTErim+FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R6";  // NOLINT
  token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";

  credential::CredentialsRedeem redeem;
  redeem.publisher_key = "brave.com";
  redeem.type = mojom::RewardsType::ONE_TIME_TIP;
  redeem.processor = mojom::ContributionProcessor::UPHOLD;
  redeem.token_list = {token};
  redeem.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
  redeem.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";

  votes_->Request(redeem, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::RETRY_SHORT);
  });
}

TEST_F(PostVotesTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  mojom::UnblindedToken token;
  token.token_value = "s1OrSZUvo/33u3Y866mQaG/b6d94TqMThLal4+DSX4UrR4jT+GtTErim+FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R6";  // NOLINT
  token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";

  credential::CredentialsRedeem redeem;
  redeem.publisher_key = "brave.com";
  redeem.type = mojom::RewardsType::ONE_TIME_TIP;
  redeem.processor = mojom::ContributionProcessor::UPHOLD;
  redeem.token_list = {token};
  redeem.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
  redeem.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";

  votes_->Request(redeem, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
  });
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
