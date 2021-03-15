/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_drain/get_drain.h"

#include <memory>
#include <string>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetDrainTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class GetDrainTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetDrain> drain_;
  const char* test_drain_id_ = "1af0bf71-c81c-4b18-9188-a0d3c4a1b53b";
  const char* drain_json_ = R"(
    {
      "drainId": "%s",
      "status": "%s"
    }
  )";

  GetDrainTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    drain_ = std::make_unique<GetDrain>(mock_ledger_impl_.get());
  }

  std::string MakeDrainBody(const std::string& url, const char* status) {
    auto params =
        base::SplitString(url, "/", base::WhitespaceHandling::TRIM_WHITESPACE,
                          base::SplitResult::SPLIT_WANT_NONEMPTY);
    auto drain_id = params[params.size() - 1];
    EXPECT_EQ(params[params.size() - 1], test_drain_id_);
    return base::StringPrintf(drain_json_, test_drain_id_, status);
  }
};

TEST_F(GetDrainTest, DrainComplete) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke([this](type::UrlRequestPtr request,
                                   client::LoadURLCallback callback) {
        type::UrlResponse response;
        response.status_code = 200;
        response.url = request->url;
        response.body = MakeDrainBody(request->url, "complete");
        callback(response);
      }));

  drain_->Request(test_drain_id_, [](const type::Result result,
                                     const type::DrainStatus status) {
    EXPECT_EQ(result, type::Result::LEDGER_OK);
    EXPECT_EQ(status, type::DrainStatus::COMPLETE);
  });
}

TEST_F(GetDrainTest, DrainPending) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke([this](type::UrlRequestPtr request,
                                   client::LoadURLCallback callback) {
        type::UrlResponse response;
        response.status_code = 200;
        response.url = request->url;
        response.body = MakeDrainBody(request->url, "pending");
        callback(response);
      }));

  drain_->Request(test_drain_id_, [](const type::Result result,
                                     const type::DrainStatus status) {
    EXPECT_EQ(result, type::Result::LEDGER_OK);
    EXPECT_EQ(status, type::DrainStatus::PENDING);
  });
}

TEST_F(GetDrainTest, DrainInvalidResponse) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke([this](type::UrlRequestPtr request,
                                   client::LoadURLCallback callback) {
        type::UrlResponse response;
        response.status_code = 200;
        response.url = request->url;
        response.body = MakeDrainBody(request->url, "thisdoesnotexist");
        callback(response);
      }));

  drain_->Request(test_drain_id_, [](const type::Result result,
                                     const type::DrainStatus status) {
    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
    EXPECT_EQ(status, type::DrainStatus::INVALID);
  });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
