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
#include "base/strings/to_string.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/thread.h"
#include "base/types/expected.h"
#include "base/types/is_instantiation.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/http/http_version.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_request.mojom-shared.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
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

template <typename T, typename E>
bool operator==(const WithHeaders<Response<T, E>>& lhs,
                const WithHeaders<Response<T, E>>& rhs) {
  if (static_cast<const Response<T, E>&>(lhs) !=
      static_cast<const Response<T, E>&>(rhs)) {
    return false;
  }

  if (!lhs.headers || !rhs.headers) {
    return lhs.headers == rhs.headers;
  }

  return lhs.headers->StrictlyEquals(*rhs.headers);
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

template <typename Request = TestEndpoint::Request,
          typename Response = TestEndpoint::Response>
struct TestCase {
  std::string test_name;
  Request request;
  net::HttpStatusCode http_status_code;
  std::string raw_response_body;
  Response expected_response;
};

template <typename Request = TestEndpoint::Request,
          typename Response = TestEndpoint::Response>
class ClientTest : public testing::TestWithParam<TestCase<Request, Response>> {
 public:
  static constexpr auto kNameGenerator = [](const auto& info) {
    return info.param.test_name;
  };

 protected:
  void Interceptor(const network::ResourceRequest& resource_request) {
    const auto& test_case = this->GetParam();

    // Always add a response immediately so the request does not hang,
    // even if validations below fail early with ADD_FAILURE().
    if constexpr (base::is_instantiation<Response, WithHeaders>) {
      auto url_response_head = network::mojom::URLResponseHead::New();
      url_response_head->headers = test_case.expected_response.headers;
      test_url_loader_factory_.AddResponse(
          resource_request.url, std::move(url_response_head),
          test_case.raw_response_body,
          network::URLLoaderCompletionStatus(
              test_case.expected_response.net_error));
    } else {
      test_url_loader_factory_.AddResponse(resource_request.url.spec(),
                                           test_case.raw_response_body,
                                           test_case.http_status_code);
    }

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
        base::JSON_PARSE_RFC);
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
    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      EXPECT_EQ(resource_request.headers.GetHeader("Authorization"),
                "Bearer 12345");
    }
  }

  void RunTestCase() {
    test_url_loader_factory_.SetInterceptor(
        base::BindRepeating(&ClientTest::Interceptor, base::Unretained(this)));

    const auto& test_case = this->GetParam();
    base::test::TestFuture<Response> future;
    Client<TestEndpoint>::Send(test_url_loader_factory_.GetSafeWeakWrapper(),
                               test_case.request, future.GetCallback());

    EXPECT_EQ(future.Take(), test_case.expected_response);
  }

  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

using ClientTestPlainRequest = ClientTest<TestEndpoint::Request>;

TEST_P(ClientTestPlainRequest, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    PlainRequest,
    ClientTestPlainRequest,
    testing::Values(
        TestCase{
            .test_name = "valid_response",
            .request = {{"some request"}},
            .http_status_code = net::HTTP_OK,
            .raw_response_body = R"({"success": "some response"})",
            .expected_response = {.net_error = net::OK,
                                  .status_code = net::HTTP_OK,
                                  .body = TestEndpoint::Response::SuccessBody(
                                      "some response")}},
        TestCase{.test_name = "invalid_response",
                 .request = {{"some request"}},
                 .http_status_code = net::HTTP_CREATED,
                 .raw_response_body = R"({"invalid": response})",
                 .expected_response = {.net_error = net::OK,
                                       .status_code = net::HTTP_CREATED,
                                       .body = std::nullopt}},
        TestCase{.test_name = "valid_error",
                 .request = {{"some request"}},
                 .http_status_code = net::HTTP_BAD_REQUEST,
                 .raw_response_body = R"({"error": "some error"})",
                 .expected_response = {.net_error = net::OK,
                                       .status_code = net::HTTP_BAD_REQUEST,
                                       .body = base::unexpected(
                                           TestEndpoint::Response::ErrorBody(
                                               "some error"))}},
        TestCase{.test_name = "invalid_error",
                 .request = {{"some request"}},
                 .http_status_code = net::HTTP_UNAUTHORIZED,
                 .raw_response_body = R"({"invalid": error})",
                 .expected_response = {.net_error = net::OK,
                                       .status_code = net::HTTP_UNAUTHORIZED,
                                       .body = std::nullopt}}),
    ClientTestPlainRequest::kNameGenerator);

using ClientTestRequestWithHeaders =
    ClientTest<WithHeaders<TestEndpoint::Request>>;

TEST_P(ClientTestRequestWithHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithHeaders,
    ClientTestRequestWithHeaders,
    testing::Values(TestCase<WithHeaders<TestEndpoint::Request>>{
        .test_name = "request_with_headers",
        .request =
            [] {
              WithHeaders<TestEndpoint::Request> request;
              request.text = "some request";
              request.headers.SetHeader("Authorization", "Bearer 12345");
              return request;
            }(),
        .http_status_code = net::HTTP_OK,
        .raw_response_body = R"({"success": "some response"})",
        .expected_response = {.net_error = net::OK,
                              .status_code = net::HTTP_OK,
                              .body = TestEndpoint::Response::SuccessBody(
                                  "some response")}}),
    ClientTestRequestWithHeaders::kNameGenerator);

using ClientTestResponseWithHeaders =
    ClientTest<TestEndpoint::Request, WithHeaders<TestEndpoint::Response>>;

TEST_P(ClientTestResponseWithHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ResponseWithHeaders,
    ClientTestResponseWithHeaders,
    testing::Values(
        TestCase<TestEndpoint::Request, WithHeaders<TestEndpoint::Response>>{
            .test_name = "response_with_headers",
            .request = {{"some request"}},
            .http_status_code = net::HTTP_OK,
            .raw_response_body = R"({"success": "some response"})",
            .expected_response =
                [] {
                  WithHeaders<TestEndpoint::Response> response;
                  response.net_error = net::OK;
                  response.status_code = net::HTTP_OK;
                  response.body =
                      TestEndpoint::Response::SuccessBody("some response");
                  response.headers =
                      net::HttpResponseHeaders::Builder({1, 1}, "200 OK")
                          .AddHeader("X-Test-Header", "test-value")
                          .Build();
                  return response;
                }()}),
    ClientTestResponseWithHeaders::kNameGenerator);

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
