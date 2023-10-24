/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_votes/post_votes.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostVotesTest.*

using ::testing::_;
using ::testing::MockFunction;

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

class PostVotesTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostVotes votes_{mock_engine_impl_};
};

TEST_F(PostVotesTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::UnblindedToken token;
  token.token_value =
      "s1OrSZUvo/33u3Y866mQaG/"
      "b6d94TqMThLal4+DSX4UrR4jT+GtTErim+"
      "FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R6";  // NOLINT
  token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";

  credential::CredentialsRedeem redeem;
  redeem.publisher_key = "brave.com";
  redeem.type = mojom::RewardsType::ONE_TIME_TIP;
  redeem.processor = mojom::ContributionProcessor::UPHOLD;
  redeem.token_list = {token};
  redeem.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
  redeem.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";

  MockFunction<PostVotesCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::OK)).Times(1);
  votes_.Request(redeem, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostVotesTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::UnblindedToken token;
  token.token_value =
      "s1OrSZUvo/33u3Y866mQaG/"
      "b6d94TqMThLal4+DSX4UrR4jT+GtTErim+"
      "FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R6";  // NOLINT
  token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";

  credential::CredentialsRedeem redeem;
  redeem.publisher_key = "brave.com";
  redeem.type = mojom::RewardsType::ONE_TIME_TIP;
  redeem.processor = mojom::ContributionProcessor::UPHOLD;
  redeem.token_list = {token};
  redeem.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
  redeem.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";

  MockFunction<PostVotesCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::RETRY_SHORT)).Times(1);
  votes_.Request(redeem, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostVotesTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::UnblindedToken token;
  token.token_value =
      "s1OrSZUvo/33u3Y866mQaG/"
      "b6d94TqMThLal4+DSX4UrR4jT+GtTErim+"
      "FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R6";  // NOLINT
  token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";

  credential::CredentialsRedeem redeem;
  redeem.publisher_key = "brave.com";
  redeem.type = mojom::RewardsType::ONE_TIME_TIP;
  redeem.processor = mojom::ContributionProcessor::UPHOLD;
  redeem.token_list = {token};
  redeem.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
  redeem.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";

  MockFunction<PostVotesCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::RETRY_SHORT)).Times(1);
  votes_.Request(redeem, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostVotesTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  mojom::UnblindedToken token;
  token.token_value =
      "s1OrSZUvo/33u3Y866mQaG/"
      "b6d94TqMThLal4+DSX4UrR4jT+GtTErim+"
      "FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R6";  // NOLINT
  token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";

  credential::CredentialsRedeem redeem;
  redeem.publisher_key = "brave.com";
  redeem.type = mojom::RewardsType::ONE_TIME_TIP;
  redeem.processor = mojom::ContributionProcessor::UPHOLD;
  redeem.token_list = {token};
  redeem.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
  redeem.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";

  MockFunction<PostVotesCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED)).Times(1);
  votes_.Request(redeem, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
