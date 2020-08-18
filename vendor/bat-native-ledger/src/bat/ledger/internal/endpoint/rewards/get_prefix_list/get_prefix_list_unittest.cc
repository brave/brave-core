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
#include "bat/ledger/internal/endpoint/rewards/get_prefix_list/get_prefix_list.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetPrefixListTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace rewards {

class GetPrefixListTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetPrefixList> list_;

  GetPrefixListTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    list_ = std::make_unique<GetPrefixList>(mock_ledger_impl_.get());
  }
};

TEST_F(GetPrefixListTest, ServerOK) {
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
            response.status_code = 200;
            response.url = url;
            response.body = "blob";
            callback(response);
          }));

  list_->Request([](const ledger::Result result, const std::string& blob) {
    EXPECT_EQ(result, ledger::Result::LEDGER_OK);
    EXPECT_EQ(blob, "blob");
  });
}

TEST_F(GetPrefixListTest, ServerErrorRandom) {
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
            response.status_code = 453;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  list_->Request([](const ledger::Result result, const std::string& blob) {
    EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
    EXPECT_EQ(blob, "");
  });
}

TEST_F(GetPrefixListTest, ServerBodyEmpty) {
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
            response.status_code = 200;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  list_->Request([](const ledger::Result result, const std::string& blob) {
    EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
    EXPECT_EQ(blob, "");
  });
}

}  // namespace rewards
}  // namespace endpoint
}  // namespace ledger
