/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_bat_loss/post_bat_loss.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/mock_ledger_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostBatLossTest.*

using ::testing::_;
using ::testing::MockFunction;

namespace brave_rewards::internal::endpoint::promotion {

class PostBatLossTest : public MockLedgerTest {
 protected:
  void SetUp() override {
    ON_CALL(mock_ledger().mock_client(), GetStringState(state::kWalletBrave, _))
        .WillByDefault([](const std::string&, auto callback) {
          std::string wallet = R"({
            "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
            "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
          })";
          std::move(callback).Run(std::move(wallet));
        });
  }

  PostBatLoss loss_;
};

TEST_F(PostBatLossTest, ServerOK) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  MockFunction<PostBatLossCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::LEDGER_OK)).Times(1);
  loss_.Request(30.0, 1, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostBatLossTest, ServerError500) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  MockFunction<PostBatLossCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::LEDGER_ERROR)).Times(1);
  loss_.Request(30.0, 1, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

TEST_F(PostBatLossTest, ServerErrorRandom) {
  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  MockFunction<PostBatLossCallback> callback;
  EXPECT_CALL(callback, Call(mojom::Result::LEDGER_ERROR)).Times(1);
  loss_.Request(30.0, 1, callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace brave_rewards::internal::endpoint::promotion
