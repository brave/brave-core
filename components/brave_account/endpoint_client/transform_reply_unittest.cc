/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/transform_reply.h"

#include <optional>

#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_account::endpoint_client {

namespace {

struct TestRequestBody {
  base::Value::Dict ToValue() const;
};

struct TestResponse {
  static std::optional<TestResponse> FromValue(const base::Value&);
};

struct TestError {
  static std::optional<TestError> FromValue(const base::Value&);
};

using TestRequest = POST<TestRequestBody>;

struct TestEndpoint {
  using Request = POST<TestRequestBody>;
  using Error = TestError;
  using Response = TestResponse;

  static GURL URL() { return GURL(); }
};

static_assert(IsEndpoint<TestEndpoint>);

using TestReply = Reply<TestEndpoint>;

}  // namespace

using TransformReplyTest = testing::Test;

TEST_F(TransformReplyTest, OnlyResponse) {
  using TransformResult = base::expected<bool, bool>;
  {
    TestReply reply = TestResponse();

    auto result = TransformReply(
        std::move(reply),
        [&](auto response) -> TransformResult { return base::ok(true); });

    static_assert(
        std::is_same_v<decltype(result), TransformResult>,
        "The TransformReply has the same return type as the Reponse handler");
    EXPECT_TRUE(result.value());
  }
  {
    TestReply reply = TestResponse();

    auto result =
        TransformReply(std::move(reply), [&](auto response) -> TransformResult {
          return base::unexpected(true);
        });

    static_assert(
        std::is_same_v<decltype(result), TransformResult>,
        "The TransformReply has the same return type as the Reponse handler");
    // Transform can produce error if there is an invalid response
    EXPECT_TRUE(result.error());
  }
  {
    TestReply reply = base::unexpected(TestError());

    // No explicit error handlers
    auto result = TransformReply(
        std::move(reply),
        [&](auto response) -> TransformResult { return base::ok(true); });

    static_assert(
        std::is_same_v<decltype(result), TransformResult>,
        "The TransformReply has the same return type as the Reponse handler");
    // Without error handler TransfromReply returns default constructed Error
    EXPECT_FALSE(result.error());
  }
}

TEST_F(TransformReplyTest, ErroHandlers) {
  using TransformResult = base::expected<std::monostate, std::string>;

  struct TestCase {
    TestReply reply;
    std::string expected_error;
  };

  const TestCase test_cases[] = {
      {base::unexpected(TestError()), "TestError"},
      {base::unexpected(NetworkError()), "NetworkError"},
      {base::unexpected(ParseError()), "ParseError"},
  };

  for (const auto& test : test_cases) {
    // All handlers
    auto result = TransformReply(
        TestReply(test.reply),
        [&](auto response) -> TransformResult {
          NOTREACHED() << "Do not call response handler on errors";
        },
        [&](NetworkError error) { return "NetworkError"; },
        [&](ParseError error) { return "ParseError"; },
        [&](TestError error) { return "TestError"; });
    EXPECT_EQ(test.expected_error, result.error());
  }

  for (const auto& test : test_cases) {
    // No explicit error handlers
    auto result = TransformReply(
        TestReply(test.reply),
        [&](auto response) -> TransformResult {
          NOTREACHED() << "Do not call response handler on errors";
        },
        [&](auto error) { return test.expected_error; });
    EXPECT_EQ(test.expected_error, result.error());
  }

  for (const auto& test : test_cases) {
    // Explicit handler only for Error
    auto result = TransformReply(
        TestReply(test.reply),
        [&](auto response) -> TransformResult {
          NOTREACHED() << "Do not call response handler on errors";
        },
        [&](TestError error) { return "TestError"; },
        [&](auto other_errors) { return "OtherError"; });

    if (test.expected_error == "TestError") {
      EXPECT_EQ(test.expected_error, result.error());
    } else {
      EXPECT_EQ("OtherError", result.error());
    }
  }
}

}  // namespace brave_account::endpoint_client
