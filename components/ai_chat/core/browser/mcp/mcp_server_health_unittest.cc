/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_server_health.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

using Status = mojom::MCPServerStatus;

TEST(MCPServerStatusTest, ForEachError) {
  constexpr std::pair<MCPErrorCode, Status> kExpected[] = {
      {MCPErrorCode::kNetworkError, Status::kNotResponding},
      {MCPErrorCode::kTimeout, Status::kNotResponding},
      {MCPErrorCode::kNoResponse, Status::kNotResponding},
      {MCPErrorCode::kSessionExpired, Status::kNotResponding},
      {MCPErrorCode::kServerError, Status::kNotResponding},
      {MCPErrorCode::kHttpError, Status::kNotResponding},
      {MCPErrorCode::kAuthRequired, Status::kAuthRequired},
      {MCPErrorCode::kAuthRejected, Status::kAuthRequired},
      {MCPErrorCode::kInvalidToken, Status::kAuthRequired},
      {MCPErrorCode::kInvalidEndpoint, Status::kUnsupported},
      {MCPErrorCode::kInvalidRequest, Status::kUnsupported},
      {MCPErrorCode::kResponseTooLarge, Status::kUnsupported},
      {MCPErrorCode::kCrossOriginRedirect, Status::kUnsupported},
      {MCPErrorCode::kStreamableHttpNotSupported, Status::kUnsupported},
      {MCPErrorCode::kUnsupportedProtocolVersion, Status::kUnsupported},
      {MCPErrorCode::kToolsNotSupported, Status::kUnsupported},
      {MCPErrorCode::kInvalidResponse, Status::kUnsupported},
      {MCPErrorCode::kHttpClientError, Status::kUnsupported},
  };
  for (const auto& [code, status] : kExpected) {
    EXPECT_EQ(GetMCPServerStatusForError(code), status)
        << static_cast<int>(code);
  }
}

class MCPServerHealthTest : public testing::Test {
 protected:
  // Fails a listing, which isn't retried until the retry is due.
  void Fail(MCPErrorCode code = MCPErrorCode::kTimeout) {
    health_.OnListingStarted();
    health_.OnListingFailed(code);
  }

  // Whether the retry is due after `delay`, and not before.
  bool RetriesAfter(base::TimeDelta delay) {
    const int retries = retries_;
    task_environment_.FastForwardBy(delay - base::Milliseconds(1));
    if (retries_ != retries) {
      return false;
    }
    task_environment_.FastForwardBy(base::Milliseconds(1));
    return retries_ == retries + 1;
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  int retries_ = 0;
  MCPServerHealth health_{
      base::BindRepeating([](int* retries) { ++*retries; }, &retries_)};
};

TEST_F(MCPServerHealthTest, StartsAvailable) {
  EXPECT_EQ(health_.status(), Status::kAvailable);
  EXPECT_FALSE(health_.IsFailing());
  EXPECT_TRUE(health_.CanReconnect());
}

TEST_F(MCPServerHealthTest, FollowsTheLastListing) {
  health_.SetInUse(true);
  Fail(MCPErrorCode::kAuthRejected);
  EXPECT_EQ(health_.status(), Status::kAuthRequired);
  EXPECT_TRUE(health_.IsFailing());
  EXPECT_TRUE(health_.is_retry_scheduled());

  health_.OnListingStarted();
  EXPECT_TRUE(health_.is_listing());
  // A listing under way isn't retried.
  EXPECT_FALSE(health_.is_retry_scheduled());
  health_.OnListingSucceeded();
  EXPECT_EQ(health_.status(), Status::kAvailable);
  EXPECT_FALSE(health_.is_retry_scheduled());
  task_environment_.FastForwardBy(kMCPMaxToolsRetryDelay * 2);
  EXPECT_EQ(retries_, 0);
}

TEST_F(MCPServerHealthTest, RetriesLessOftenWhileFailing) {
  health_.SetInUse(true);
  Fail();
  for (base::TimeDelta delay :
       {base::Minutes(1), base::Minutes(2), base::Minutes(4), base::Minutes(8),
        kMCPMaxToolsRetryDelay, kMCPMaxToolsRetryDelay}) {
    SCOPED_TRACE(delay);
    ASSERT_TRUE(RetriesAfter(delay));
    Fail();
  }
}

TEST_F(MCPServerHealthTest, RetriesOnlyWhileInUse) {
  Fail();
  task_environment_.FastForwardBy(kMCPMaxToolsRetryDelay * 2);
  EXPECT_EQ(retries_, 0);

  // A retry that came due meanwhile is made once it's used.
  health_.SetInUse(true);
  task_environment_.FastForwardBy(base::TimeDelta());
  EXPECT_EQ(retries_, 1);
  Fail();
  health_.SetInUse(false);
  task_environment_.FastForwardBy(kMCPMaxToolsRetryDelay * 2);
  EXPECT_EQ(retries_, 1);
}

TEST_F(MCPServerHealthTest, LateListingDecidesTheStatus) {
  health_.SetInUse(true);
  health_.OnListingStarted();
  health_.OnListingLate();
  EXPECT_EQ(health_.status(), Status::kNotResponding);
  EXPECT_TRUE(health_.IsFailing());
  // Retried once it ends, if it fails.
  EXPECT_FALSE(health_.is_retry_scheduled());
  health_.OnListingSucceeded();
  EXPECT_EQ(health_.status(), Status::kAvailable);

  health_.OnListingStarted();
  health_.OnListingLate();
  health_.OnListingFailed(MCPErrorCode::kAuthRequired);
  EXPECT_EQ(health_.status(), Status::kAuthRequired);
  EXPECT_TRUE(RetriesAfter(kMCPToolsRetryDelay));
}

TEST_F(MCPServerHealthTest, CheckingForgetsEarlierFailures) {
  health_.SetInUse(true);
  Fail();
  ASSERT_TRUE(RetriesAfter(base::Minutes(1)));
  Fail();

  health_.StartChecking();
  EXPECT_EQ(health_.status(), Status::kChecking);
  EXPECT_FALSE(health_.IsFailing());
  EXPECT_FALSE(health_.is_retry_scheduled());
  Fail();
  EXPECT_TRUE(RetriesAfter(kMCPToolsRetryDelay));
}

TEST_F(MCPServerHealthTest, ReconnectsAreSpacedOut) {
  health_.OnListingStarted();
  EXPECT_FALSE(health_.CanReconnect());
  health_.OnListingFailed(MCPErrorCode::kTimeout);
  task_environment_.FastForwardBy(kMCPMinReconnectInterval -
                                  base::Milliseconds(1));
  EXPECT_FALSE(health_.CanReconnect());
  task_environment_.FastForwardBy(base::Milliseconds(1));
  EXPECT_TRUE(health_.CanReconnect());
}

}  // namespace ai_chat
