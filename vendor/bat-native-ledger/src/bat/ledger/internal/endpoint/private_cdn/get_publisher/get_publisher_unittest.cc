/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/endpoint/private_cdn/get_publisher/get_publisher.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetPublisherTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace private_cdn {

class GetPublisherTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetPublisher> publisher_;

  GetPublisherTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    publisher_ = std::make_unique<GetPublisher>(mock_ledger_impl_.get());
  }
};

TEST_F(GetPublisherTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 404;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  publisher_->Request(
      "brave.com",
      "ce55",
      [](const ledger::Result result, ledger::ServerPublisherInfoPtr info) {
    EXPECT_EQ(result, ledger::Result::LEDGER_OK);
    EXPECT_EQ(info->publisher_key, "brave.com");
    EXPECT_EQ(info->status, ledger::PublisherStatus::NOT_VERIFIED);
  });
}

}  // namespace private_cdn
}  // namespace endpoint
}  // namespace ledger
