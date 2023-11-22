/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/rewards/get_prefix_list/get_prefix_list.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetPrefixListTest.*

using ::testing::_;
using ::testing::MockFunction;

namespace brave_rewards::internal {
namespace endpoint {
namespace rewards {

class GetPrefixListTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  GetPrefixList list_{mock_engine_impl_};
};

TEST_F(GetPrefixListTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "blob";
        std::move(callback).Run(std::move(response));
      });

  MockFunction<GetPrefixListCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::OK, "blob")).Times(1);
  list_.Request(callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(GetPrefixListTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  MockFunction<GetPrefixListCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED, "")).Times(1);
  list_.Request(callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(GetPrefixListTest, ServerBodyEmpty) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  MockFunction<GetPrefixListCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::FAILED, "")).Times(1);
  list_.Request(callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace rewards
}  // namespace endpoint
}  // namespace brave_rewards::internal
