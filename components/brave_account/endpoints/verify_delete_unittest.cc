/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/verify_delete.h"

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const VerifyDelete::Response::SuccessBody&,
                const VerifyDelete::Response::SuccessBody&) {
  return true;
}

bool operator==(const VerifyDelete::Response::ErrorBody&,
                const VerifyDelete::Response::ErrorBody&) {
  return true;
}

namespace {

using VerifyDeleteTestCase = EndpointTestCase<VerifyDelete>;

const VerifyDeleteTestCase* Success() {
  static const base::NoDestructor<VerifyDeleteTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_NO_CONTENT,
       .raw_response_body = "",
       .expected_response = {.net_error = net::OK,
                             .status_code = net::HTTP_NO_CONTENT,
                             .body = VerifyDelete::Response::SuccessBody()}});

  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": null, "error": "Bad Request", "status": 400 }
//   - { "code": 13003, "error": "intent not allowed", "status": 400 }
//   - { "code": 13009, "error": "email already verified", "status": 400 }
// - HTTP 401:
//   - { "code": null, "error": "Unauthorized", "status": 401 }
// - HTTP 5XX:
//   - { "code": null, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const VerifyDeleteTestCase* ApplicationJsonError() {
  static const base::NoDestructor<VerifyDeleteTestCase> kApplicationJsonError(
      {.test_name = "application_json_error",
       .http_status_code = net::HTTP_BAD_REQUEST,
       .raw_response_body =
           R"({ "code": null,
                "error": "Bad Request",
                "status": 400 })",
       .expected_response = {
           .net_error = net::OK,
           .status_code = net::HTTP_BAD_REQUEST,
           .body = base::unexpected(VerifyDelete::Response::ErrorBody())}});
  return kApplicationJsonError.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const VerifyDeleteTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<VerifyDeleteTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_response_body = "non-application/json error",
           .expected_response = {
               .net_error = net::OK,
               .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
               .body = base::unexpected(VerifyDelete::Response::ErrorBody())}});
  return kNonApplicationJsonError.get();
}

using VerifyDeleteTest = EndpointTest<VerifyDelete>;

}  // namespace

TEST_P(VerifyDeleteTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(VerifyDeleteTestCases,
                         VerifyDeleteTest,
                         testing::Values(Success(),
                                         ApplicationJsonError(),
                                         NonApplicationJsonError()),
                         VerifyDeleteTest::kNameGenerator);

}  // namespace brave_account::endpoints
