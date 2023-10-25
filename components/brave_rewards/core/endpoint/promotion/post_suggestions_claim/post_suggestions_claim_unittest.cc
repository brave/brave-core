/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_suggestions_claim/post_suggestions_claim.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostSuggestionsClaimTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

class PostSuggestionsClaimTest : public testing::Test {
 protected:
  PostSuggestionsClaimTest() {
    mojom::UnblindedToken token;
    token.token_value =
        "s1OrSZUvo/33u3Y866mQaG/"
        "b6d94TqMThLal4+DSX4UrR4jT+GtTErim+"
        "FtEyZ7nebNGRoUDxObiUni9u8BB0DIT2aya6rYWko64IrXJWpbf0SVHnQFVYNyX64NjW9R"
        "6";  // NOLINT
    token.public_key = "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=";
    redeem_.publisher_key = "brave.com";
    redeem_.type = mojom::RewardsType::ONE_TIME_TIP;
    redeem_.processor = mojom::ContributionProcessor::BRAVE_TOKENS;
    redeem_.token_list = {token};
    redeem_.order_id = "c4645786-052f-402f-8593-56af2f7a21ce";
    redeem_.contribution_id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";
  }

  void SetUp() override {
    ON_CALL(*mock_engine_impl_.mock_client(),
            GetStringState(state::kWalletBrave, _))
        .WillByDefault([](const std::string&, auto callback) {
          std::string wallet = R"({
            "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
            "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
          })";
          std::move(callback).Run(std::move(wallet));
        });
  }

  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PostSuggestionsClaim claim_{mock_engine_impl_};
  credential::CredentialsRedeem redeem_;
};

TEST_F(PostSuggestionsClaimTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"(
              {"drainId": "1af0bf71-c81c-4b18-9188-a0d3c4a1b53b"}
            )";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostSuggestionsClaimCallback> callback;
  EXPECT_CALL(callback,
              Run(mojom::Result::OK, "1af0bf71-c81c-4b18-9188-a0d3c4a1b53b"))
      .Times(1);
  claim_.Request(redeem_, callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostSuggestionsClaimTest, ServerNeedsRetry) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostSuggestionsClaimCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, "")).Times(1);
  claim_.Request(redeem_, callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostSuggestionsClaimTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostSuggestionsClaimCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, "")).Times(1);
  claim_.Request(redeem_, callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PostSuggestionsClaimTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PostSuggestionsClaimCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, "")).Times(1);
  claim_.Request(redeem_, callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
