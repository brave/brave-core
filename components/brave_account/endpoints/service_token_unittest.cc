/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/service_token.h"

#include <string>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const ServiceToken::Response& lhs,
                const ServiceToken::Response& rhs) {
  return lhs.auth_token == rhs.auth_token;
}

using ServiceTokenTestCase = EndpointTestCase<ServiceToken>;

namespace {

const ServiceTokenTestCase* Success() {
  static const base::NoDestructor<ServiceTokenTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_OK,
       .raw_reply = R"({ "authToken": "34c375d933e3c" })",
       .reply = [] {
         ServiceToken::Response response;
         response.auth_token = "34c375d933e3c";
         return response;
       }()});
  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": 0, "error": "Bad Request", "status": 400 }
//   - { "code": 13006, "error": "email domain is not supported", "status": 400 }
// - HTTP 401:
//   - { "code": 0, "error": "Unauthorized", "status": 401 }
// - HTTP 403:
//   - { "code": 14004, "error": "incorrect credentials", "status": 403 }
//   - { "code": 14007, "error": "invalid token audience", "status": 403 }
// - HTTP 5XX:
//   - { "code": 0, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const ServiceTokenTestCase* ApplicationJsonError() {
  static const base::NoDestructor<ServiceTokenTestCase> kApplicationJsonError(
      {.test_name = "application_json_error",
       .http_status_code = net::HTTP_BAD_REQUEST,
       .raw_reply =
           R"({ "code": 13006,
                "error": "email domain is not supported",
                "status": 400 })",
       .reply = base::unexpected([] {
         ServiceToken::Error error;
         error.code = base::Value(13006);
         return error;
       }())});
  return kApplicationJsonError.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const ServiceTokenTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<ServiceTokenTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_reply = "non-application/json error",
           .reply = base::unexpected(std::nullopt)});
  return kNonApplicationJsonError.get();
}

}  // namespace

using ServiceTokenTest = EndpointTest<ServiceToken>;

TEST_P(ServiceTokenTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(ServiceTokenTestCases,
                         ServiceTokenTest,
                         testing::Values(Success(),
                                         ApplicationJsonError(),
                                         NonApplicationJsonError()),
                         ServiceTokenTest::kNameGenerator);

}  // namespace brave_account::endpoints
