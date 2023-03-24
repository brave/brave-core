/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/post_recipient_id/post_recipient_id_gemini.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostRecipientIdTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace gemini {

class GeminiPostRecipientIdTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  PostRecipientId post_recipient_id_{&mock_ledger_impl_};
};

TEST_F(GeminiPostRecipientIdTest, ServerOK) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"({
              "result": "OK",
              "recipient_id": "60f9be89-ada7-486d-9cef-f6d3a10886d7",
              "label": "deposit_address"
            })";
            std::move(callback).Run(response);
          }));

  post_recipient_id_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& recipient_id) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(recipient_id, "60f9be89-ada7-486d-9cef-f6d3a10886d7");
      }));
}

TEST_F(GeminiPostRecipientIdTest, ServerOK_Unverified) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"({
              "result": "OK",
              "recipient_id": "60f9be89-ada7-486d-9cef-f6d3a10886d7",
              "label": "deposit_address"
            })";
            response.headers.insert(std::pair<std::string, std::string>(
                "www-authenticate", "Bearer error=\"unverified_account\""));
            std::move(callback).Run(response);
          }));

  post_recipient_id_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& recipient_id) {
        EXPECT_EQ(result, mojom::Result::NOT_FOUND);
        EXPECT_EQ(recipient_id, "");
      }));
}

TEST_F(GeminiPostRecipientIdTest, ServerError401) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  post_recipient_id_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& recipient_id) {
        EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
        EXPECT_EQ(recipient_id, "");
      }));
}

TEST_F(GeminiPostRecipientIdTest, ServerError403) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_FORBIDDEN;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  post_recipient_id_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& recipient_id) {
        EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
        EXPECT_EQ(recipient_id, "");
      }));
}

TEST_F(GeminiPostRecipientIdTest, ServerError404) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_NOT_FOUND;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  post_recipient_id_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& recipient_id) {
        EXPECT_EQ(result, mojom::Result::NOT_FOUND);
        EXPECT_EQ(recipient_id, "");
      }));
}

TEST_F(GeminiPostRecipientIdTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 418;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  post_recipient_id_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& recipient_id) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
        EXPECT_EQ(recipient_id, "");
      }));
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
