/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/rewards/get_prefix_list/get_prefix_list.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetPrefixListTest.*

using ::testing::_;

namespace ledger {
namespace endpoint {
namespace rewards {

class GetPrefixListTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  GetPrefixList list_{&mock_ledger_impl_};
};

TEST_F(GetPrefixListTest, ServerOK) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = 200;
            response->url = request->url;
            response->body = "blob";
            std::move(callback).Run(std::move(response));
          });

  list_.Request([](const mojom::Result result, const std::string& blob) {
    EXPECT_EQ(result, mojom::Result::LEDGER_OK);
    EXPECT_EQ(blob, "blob");
  });
}

TEST_F(GetPrefixListTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = 453;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  list_.Request([](const mojom::Result result, const std::string& blob) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
    EXPECT_EQ(blob, "");
  });
}

TEST_F(GetPrefixListTest, ServerBodyEmpty) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = 200;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  list_.Request([](const mojom::Result result, const std::string& blob) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
    EXPECT_EQ(blob, "");
  });
}

}  // namespace rewards
}  // namespace endpoint
}  // namespace ledger
