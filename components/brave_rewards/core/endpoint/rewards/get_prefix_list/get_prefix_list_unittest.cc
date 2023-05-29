/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/rewards/get_prefix_list/get_prefix_list.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/test/mock_ledger_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetPrefixListTest.*

using ::testing::_;
using ::testing::MockFunction;

namespace brave_rewards::internal::endpoint::rewards {

class GetPrefixListTest : public MockLedgerTest {
 protected:
  GetPrefixList list_;
};

TEST_F(GetPrefixListTest, ServerOK) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "blob";
        std::move(callback).Run(std::move(response));
      });

  MockFunction<GetPrefixListCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::LEDGER_OK, "blob")).Times(1);
  list_.Request(callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(GetPrefixListTest, ServerErrorRandom) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  MockFunction<GetPrefixListCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::LEDGER_ERROR, "")).Times(1);
  list_.Request(callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(GetPrefixListTest, ServerBodyEmpty) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  MockFunction<GetPrefixListCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::LEDGER_ERROR, "")).Times(1);
  list_.Request(callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace brave_rewards::internal::endpoint::rewards
