/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/gemini/post_recipient_id/post_recipient_id_gemini.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostRecipientIdTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace gemini {

class GeminiPostRecipientIdTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostRecipientId> post_recipient_id_;

  GeminiPostRecipientIdTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    post_recipient_id_ =
        std::make_unique<PostRecipientId>(mock_ledger_impl_.get());
  }
};

TEST_F(GeminiPostRecipientIdTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"({
              "result": "OK",
              "recipient_id": "60f9be89-ada7-486d-9cef-f6d3a10886d7",
              "label": "deposit_address"
            })";
            callback(response);
          }));

  post_recipient_id_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& recipient_id) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(recipient_id, "60f9be89-ada7-486d-9cef-f6d3a10886d7");
      });
}

TEST_F(GeminiPostRecipientIdTest, ServerOK_Unverified) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"({
              "result": "OK",
              "recipient_id": "60f9be89-ada7-486d-9cef-f6d3a10886d7",
              "label": "deposit_address"
            })";
            response.headers.insert(std::pair<std::string, std::string>(
                "www-authenticate", "Bearer error=\"unverified_account\""));
            callback(response);
          }));

  post_recipient_id_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& recipient_id) {
        EXPECT_EQ(result, type::Result::NOT_FOUND);
        EXPECT_EQ(recipient_id, "");
      });
}

TEST_F(GeminiPostRecipientIdTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_recipient_id_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& recipient_id) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
        EXPECT_EQ(recipient_id, "");
      });
}

TEST_F(GeminiPostRecipientIdTest, ServerError403) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_FORBIDDEN;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_recipient_id_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string recipient_id) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
        EXPECT_EQ(recipient_id, "");
      });
}

TEST_F(GeminiPostRecipientIdTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_NOT_FOUND;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_recipient_id_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string recipient_id) {
        EXPECT_EQ(result, type::Result::NOT_FOUND);
        EXPECT_EQ(recipient_id, "");
      });
}

TEST_F(GeminiPostRecipientIdTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 418;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_recipient_id_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& recipient_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(recipient_id, "");
      });
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
