/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/client.h"

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/to_string.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/thread.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_request.mojom-shared.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

template <const char* Key>
struct Body {
  base::Value::Dict ToValue() const {
    return base::Value::Dict().Set(Key, text);
  }

  static std::optional<Body> FromValue(const base::Value& value) {
    const auto* dict = value.GetIfDict();
    if (!dict) {
      return std::nullopt;
    }

    const auto* found = dict->FindString(Key);
    if (!found) {
      return std::nullopt;
    }

    return Body(*found);
  }

  bool operator==(const Body& other) const { return text == other.text; }

  std::string text;
};

inline constexpr char kRequestKey[] = "request";
inline constexpr char kSuccessKey[] = "success";
inline constexpr char kErrorKey[] = "error";

}  // namespace

namespace brave_account::endpoint_client {

template <typename T, typename E>
bool operator==(const Response<T, E>& lhs, const Response<T, E>& rhs) {
  return lhs.net_error == rhs.net_error && lhs.status_code == rhs.status_code &&
         lhs.body == rhs.body;
}

using TestRequestBody = Body<kRequestKey>;
using TestRequest = POST<TestRequestBody>;

using TestSuccessBody = Body<kSuccessKey>;
using TestErrorBody = Body<kErrorKey>;
using TestResponse = Response<TestSuccessBody, TestErrorBody>;

// Requests look like this:
// POST https://example.com/api/query
// {
//   "request": "what the client wants"
// }

// Successful responses look like this:
// {
//   "success": "the answer is 42"
// }

// Errors look like this:
// {
//   "error": "insert a quarter to continue"
// }
struct TestEndpoint {
  using Request = TestRequest;
  using Response = TestResponse;

  static GURL URL() { return GURL("https://example.com/api/query"); }
};

struct TestCase {
  TestEndpoint::Request request;
  bool with_headers;
  net::HttpStatusCode http_status_code;
  std::string raw_response_body;
  TestEndpoint::Response expected_response;
};

class ClientTest : public testing::TestWithParam<TestCase> {
 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

TEST_P(ClientTest, Send) {
  const TestCase& test_case = GetParam();

  test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&](const network::ResourceRequest& resource_request) {
        // Always add a response immediately so the request does not hang,
        // even if validations below fail early with ADD_FAILURE().
        test_url_loader_factory_.AddResponse(resource_request.url.spec(),
                                             test_case.raw_response_body,
                                             test_case.http_status_code);

        // Method
        EXPECT_EQ(resource_request.method, "POST");
        // URL
        EXPECT_EQ(resource_request.url, GURL("https://example.com/api/query"));
        // Request body
        if (!resource_request.request_body) {
          return ADD_FAILURE() << "resource_request.request_body is nullptr!";
        }
        const auto* elements = resource_request.request_body->elements();
        if (!elements) {
          return ADD_FAILURE() << "elements is nullptr!";
        }
        EXPECT_EQ(elements->size(), 1u);
        const auto& element = elements->front();
        EXPECT_EQ(element.type(), network::DataElement::Tag::kBytes);
        const auto body = base::JSONReader::Read(
            element.As<network::DataElementBytes>().AsStringPiece(),
            base::JSON_PARSE_CHROMIUM_EXTENSIONS);
        if (!body || !body->is_dict()) {
          return ADD_FAILURE()
                 << "body is std::nullopt or not a base::Value::Dict!";
        }
        const auto& dict = body->GetDict();
        const auto* request = dict.FindString("request");
        EXPECT_NE(request, nullptr);
        EXPECT_EQ(*request, test_case.request.text);
        // Headers
        EXPECT_EQ(resource_request.headers.GetHeader("Content-Type"),
                  "application/json");
        if (test_case.with_headers) {
          EXPECT_EQ(resource_request.headers.GetHeader("Authorization"),
                    "Bearer 12345");
        }
      }));

  base::test::TestFuture<TestEndpoint::Response> future;
  if (test_case.with_headers) {
    WithHeaders<TestEndpoint::Request> request{test_case.request};
    request.headers.SetHeader("Authorization", "Bearer 12345");
    Client<TestEndpoint>::Send(test_url_loader_factory_.GetSafeWeakWrapper(),
                               std::move(request), future.GetCallback());
  } else {
    Client<TestEndpoint>::Send(test_url_loader_factory_.GetSafeWeakWrapper(),
                               test_case.request, future.GetCallback());
  }

  EXPECT_EQ(future.Take(), test_case.expected_response);
}

INSTANTIATE_TEST_SUITE_P(
    ClientTestCases,
    ClientTest,
    testing::Values(
        TestCase{
            .request = TestEndpoint::Request{{"valid response"}},
            .with_headers = false,
            .http_status_code = net::HTTP_OK,
            .raw_response_body = R"({"success": "some response"})",
            .expected_response = {.net_error = net::OK,
                                  .status_code = net::HTTP_OK,
                                  .body = TestEndpoint::Response::SuccessBody(
                                      "some response")}},
        TestCase{.request = TestEndpoint::Request{{"invalid response"}},
                 .with_headers = false,
                 .http_status_code = net::HTTP_CREATED,
                 .raw_response_body = R"({"invalid": response})",
                 .expected_response = {.net_error = net::OK,
                                       .status_code = net::HTTP_CREATED,
                                       .body = std::nullopt}},
        TestCase{.request = TestEndpoint::Request{{"valid error"}},
                 .with_headers = false,
                 .http_status_code = net::HTTP_BAD_REQUEST,
                 .raw_response_body = R"({"error": "some error"})",
                 .expected_response = {.net_error = net::OK,
                                       .status_code = net::HTTP_BAD_REQUEST,
                                       .body = base::unexpected(
                                           TestEndpoint::Response::ErrorBody(
                                               "some error"))}},
        TestCase{.request = TestEndpoint::Request{{"invalid error"}},
                 .with_headers = false,
                 .http_status_code = net::HTTP_UNAUTHORIZED,
                 .raw_response_body = R"({"invalid": error})",
                 .expected_response = {.net_error = net::OK,
                                       .status_code = net::HTTP_UNAUTHORIZED,
                                       .body = std::nullopt}},
        TestCase{
            .request = TestEndpoint::Request{{"request with headers"}},
            .with_headers = true,
            .http_status_code = net::HTTP_OK,
            .raw_response_body = R"({"success": "some response"})",
            .expected_response = {.net_error = net::OK,
                                  .status_code = net::HTTP_OK,
                                  .body = TestEndpoint::Response::SuccessBody(
                                      "some response")}}),
    [](const auto& info) {
      std::string name;
      base::ReplaceChars(
          info.param.request.text + "_HTTP_" +
              base::ToString(info.param.http_status_code) + "_" +
              net::GetHttpReasonPhrase(info.param.http_status_code),
          " ", "_", &name);
      return name;
    });

namespace {

enum class CancelRequestOn { kSameSequence, kDifferentSequence };

class ClientCancelTest : public testing::TestWithParam<CancelRequestOn> {
 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

}  // namespace

TEST_P(ClientCancelTest, Cancel) {
  base::test::TestFuture<TestEndpoint::Response> future;
  RequestHandle request_handle =
      Client<TestEndpoint>::Send<RequestCancelability::kCancelable>(
          test_url_loader_factory_.GetSafeWeakWrapper(),
          TestEndpoint::Request{{"cancel me"}}, future.GetCallback());

  auto weak_simple_url_loader =
      CHECK_DEREF(static_cast<network::SimpleURLLoader*>(request_handle.get()))
          .GetWeakPtr();
  EXPECT_TRUE(weak_simple_url_loader);

  // We intentionally don't add a response for this request.
  // It will hang, so the only way it completes is
  // by explicitly canceling via |request_handle|.
  if (GetParam() == CancelRequestOn::kSameSequence) {
    request_handle.reset();
  } else {
    base::Thread thread("cancel-thread");
    CHECK(thread.Start());
    thread.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](RequestHandle request_handle) { request_handle.reset(); },
            std::move(request_handle)));
    thread.FlushForTesting();  // ensures request_handle.reset() ran
  }

  // The deleter uses `SequencedTaskRunner::DeleteSoon()`
  // on the owning sequence, so wait for it to run.
  EXPECT_TRUE(base::test::RunUntil([&] { return !weak_simple_url_loader; }));
  EXPECT_FALSE(future.IsReady());
}

INSTANTIATE_TEST_SUITE_P(Cancelation,
                         ClientCancelTest,
                         testing::Values(CancelRequestOn::kSameSequence,
                                         CancelRequestOn::kDifferentSequence),
                         [](const auto& info) {
                           return info.param == CancelRequestOn::kSameSequence
                                      ? "SameSequence"
                                      : "DifferentSequence";
                         });

}  // namespace brave_account::endpoint_client
