/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/client.h"

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "base/json/json_reader.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_util.h"
#include "base/strings/to_string.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_request.mojom-shared.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {
template <const char* Key>
struct Message {
  base::Value::Dict ToValue() const {
    return base::Value::Dict().Set(Key, text);
  }

  static std::optional<Message> FromValue(const base::Value& value) {
    const auto* dict = value.GetIfDict();
    if (!dict) {
      return std::nullopt;
    }

    const auto* found = dict->FindString(Key);
    if (!found) {
      return std::nullopt;
    }

    return Message(*found);
  }

  bool operator==(const Message& other) const { return text == other.text; }

  std::string text;
};

inline constexpr char kRequestKey[] = "request";
inline constexpr char kResponseKey[] = "response";
inline constexpr char kErrorKey[] = "error";
}  // namespace

namespace brave_account::endpoint_client {

using TestRequestBody = Message<kRequestKey>;
using TestRequest = POST<TestRequestBody>;
using TestResponse = Message<kResponseKey>;
using TestError = Message<kErrorKey>;

// Requests look like this:
// POST https://example.com/api/query
// {
//   "request": "what the client wants"
// }

// Responses look like this:
// {
//   "response": "the answer is 42"
// }

// Errors look like this:
// {
//   "error": "insert a quarter to continue"
// }
struct TestEndpoint {
  using Request = TestRequest;
  using Response = TestResponse;
  using Error = TestError;
  static GURL URL() { return GURL("https://example.com/api/query"); }
};

using Expected =
    base::expected<std::optional<TestResponse>, std::optional<TestError>>;

struct TestCase {
  TestRequest request;
  bool with_headers;
  net::HttpStatusCode status_code;
  std::string server_reply;
  Expected expected_reply;
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
                                             test_case.server_reply,
                                             test_case.status_code);

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
            element.As<network::DataElementBytes>().AsStringPiece());
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

  base::test::TestFuture<int, Expected> future;
  if (test_case.with_headers) {
    WithHeaders<TestRequest> request{test_case.request};
    request.headers.SetHeader("Authorization", "Bearer 12345");
    Client<TestEndpoint>::Send(test_url_loader_factory_.GetSafeWeakWrapper(),
                               std::move(request), future.GetCallback());
  } else {
    Client<TestEndpoint>::Send(test_url_loader_factory_.GetSafeWeakWrapper(),
                               test_case.request, future.GetCallback());
  }

  EXPECT_EQ(future.Take(),
            std::tie(test_case.status_code, test_case.expected_reply));
}

INSTANTIATE_TEST_SUITE_P(
    ClientTestCases,
    ClientTest,
    testing::Values(
        TestCase{.request = TestRequest{{"valid response"}},
                 .with_headers = false,
                 .status_code = net::HTTP_OK,
                 .server_reply = R"({"response": "some response"})",
                 .expected_reply = TestResponse("some response")},
        TestCase{.request = TestRequest{{"invalid response"}},
                 .with_headers = false,
                 .status_code = net::HTTP_CREATED,
                 .server_reply = R"({"invalid": response})",
                 .expected_reply = Expected(std::nullopt)},
        TestCase{.request = TestRequest{{"valid error"}},
                 .with_headers = false,
                 .status_code = net::HTTP_BAD_REQUEST,
                 .server_reply = R"({"error": "some error"})",
                 .expected_reply = base::unexpected(TestError("some error"))},
        TestCase{.request = TestRequest{{"invalid error"}},
                 .with_headers = false,
                 .status_code = net::HTTP_UNAUTHORIZED,
                 .server_reply = R"({"invalid": error})",
                 .expected_reply = base::unexpected(std::nullopt)},
        TestCase{.request = TestRequest{{"request with headers"}},
                 .with_headers = true,
                 .status_code = net::HTTP_OK,
                 .server_reply = R"({"response": "some response"})",
                 .expected_reply = TestResponse("some response")}),
    [](const auto& info) {
      std::string name;
      base::ReplaceChars(info.param.request.text + "_HTTP_" +
                             base::ToString(info.param.status_code) + "_" +
                             net::GetHttpReasonPhrase(info.param.status_code),
                         " ", "_", &name);
      return name;
    });

}  // namespace brave_account::endpoint_client
