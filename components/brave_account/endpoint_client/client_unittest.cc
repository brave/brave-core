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
#include "brave/components/brave_account/endpoint_client/static_string.h"
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
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

namespace brave_account::endpoint_client {

template <typename T, typename E>
bool operator==(const Response<T, E>& lhs, const Response<T, E>& rhs) {
  return std::tie(lhs.net_error, lhs.status_code, lhs.body) ==
         std::tie(rhs.net_error, rhs.status_code, rhs.body);
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

namespace {

template <detail::StaticString Key>
struct Body {
  base::Value::Dict ToValue() const {
    return base::Value::Dict().Set(Key.value, text);
  }

  static std::optional<Body> FromValue(const base::Value& value) {
    const auto* dict = value.GetIfDict();
    if (!dict) {
      return std::nullopt;
    }

    const auto* found = dict->FindString(Key.value);
    if (!found) {
      return std::nullopt;
    }

    return Body(*found);
  }

  bool operator==(const Body& other) const { return text == other.text; }

  std::string text;
};

template <>
struct Body<""> {
  base::Value::Dict ToValue() const { return base::Value::Dict(); }

  static std::optional<Body> FromValue(const base::Value& value) {
    return value.is_dict() ? Body() : std::optional<Body>();
  }

  bool operator==(const Body&) const { return true; }
};

inline constexpr char kRequestKey[] = "request";
inline constexpr char kSuccessKey[] = "success";
inline constexpr char kErrorKey[] = "error";
inline constexpr char kEmptyKey[] = "";

using TestRequestBody = Body<kRequestKey>;
using TestSuccessBody = Body<kSuccessKey>;
using TestErrorBody = Body<kErrorKey>;
using TestEmptyBody = Body<kEmptyKey>;

template <typename ResponseT>
struct Endpoint {
  using Request = POST<TestRequestBody>;
  using Response = ResponseT;
  static GURL URL() { return GURL("https://example.com/api/query"); }
};

// Request: POST https://example.com/api/query { "request": "..." }
// Success: { "success": "..." }
// Error:   { "error": "..." }
using TestEndpoint = Endpoint<Response<TestSuccessBody, TestErrorBody>>;

// Request: POST https://example.com/api/query { "request": "..." }
// Success: ignored
// Error:   { "error": "..." }
using TestEndpointWithEmptySuccessBody =
    Endpoint<Response<TestEmptyBody, TestErrorBody>>;

// Request: POST https://example.com/api/query { "request": "..." }
// Success: { "success": "..." }
// Error:   ignored
using TestEndpointWithEmptyErrorBody =
    Endpoint<Response<TestSuccessBody, TestEmptyBody>>;

template <typename Request, typename Response>
struct TestCase {
  std::string test_name;
  Request request;
  int net_error;
  std::optional<net::HttpStatusCode> status_code;
  std::string body;
  Response expected_response;
};

template <typename Endpoint,
          typename Request = typename Endpoint::Request,
          typename Response = typename Endpoint::Response>
class ClientTest : public testing::TestWithParam<TestCase<Request, Response>> {
 public:
  using TestCase = typename ClientTest::ParamType;

  static constexpr auto kNameGenerator = [](const auto& info) {
    return info.param.test_name;
  };

  static std::vector<TestCase> GenerateTestCases() {
    auto make_request = [] {
      Request request;
      request.text = "request";

      if constexpr (base::is_instantiation<Request, WithHeaders>) {
        SetBearerToken(request, "12345");
      }

      return request;
    };

    auto make_response = [](int net_error,
                            std::optional<net::HttpStatusCode> status_code,
                            auto body) {
      Response response;
      response.net_error = net_error;
      response.status_code = status_code;
      response.body = std::move(body);

      if constexpr (base::is_instantiation<Response, WithHeaders>) {
        if (status_code) {
          const char* phrase = net::GetHttpReasonPhrase(*status_code);
          CHECK(phrase);

          response.headers =
              net::HttpResponseHeaders::Builder(
                  {1, 1}, absl::StrFormat("%d %s", *status_code, phrase))
                  .AddHeader("X-Test-Header", "test-value")
                  .Build();
        }
      }

      return response;
    };

    auto make_success_body = [](bool non_empty_nullopt) {
      using SuccessBody = typename Endpoint::Response::SuccessBody;

      if constexpr (std::is_empty_v<SuccessBody>) {
        return SuccessBody();
      } else {
        return non_empty_nullopt ? std::optional<SuccessBody>()
                                 : SuccessBody("success");
      }
    };

    auto make_error_body = [](bool non_empty_nullopt) {
      using ErrorBody = typename Endpoint::Response::ErrorBody;

      if constexpr (std::is_empty_v<ErrorBody>) {
        return base::unexpected(ErrorBody());
      } else {
        return non_empty_nullopt ? std::optional<base::unexpected<ErrorBody>>()
                                 : base::unexpected(ErrorBody("error"));
      }
    };

    return {
        TestCase{.test_name = "net_error",
                 .request = make_request(),
                 .net_error = net::ERR_FAILED,
                 .status_code = std::nullopt,
                 .body = "",
                 .expected_response = make_response(
                     net::ERR_FAILED, std::nullopt, std::nullopt)},
        TestCase{.test_name = "no_response_headers",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = std::nullopt,
                 .body = "",
                 .expected_response =
                     make_response(net::OK, std::nullopt, std::nullopt)},
        TestCase{.test_name = "success_empty_response",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_OK,
                 .body = "",
                 .expected_response = make_response(net::OK, net::HTTP_OK,
                                                    make_success_body(true))},
        TestCase{.test_name = "success_plain_text_response",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_OK,
                 .body = "plain text",
                 .expected_response = make_response(net::OK, net::HTTP_OK,
                                                    make_success_body(true))},
        TestCase{.test_name = "success_invalid_json_response",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_OK,
                 .body = R"({"invalid": json})",
                 .expected_response = make_response(net::OK, net::HTTP_OK,
                                                    make_success_body(true))},
        TestCase{.test_name = "success_valid_json_lacks_success_field",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_OK,
                 .body = R"({"valid": "json"})",
                 .expected_response = make_response(net::OK, net::HTTP_OK,
                                                    make_success_body(true))},
        TestCase{.test_name = "success_valid_json_has_success_field",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_OK,
                 .body = R"({"success": "success"})",
                 .expected_response = make_response(net::OK, net::HTTP_OK,
                                                    make_success_body(false))},
        TestCase{.test_name = "error_empty_response",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_BAD_REQUEST,
                 .body = "",
                 .expected_response = make_response(
                     net::OK, net::HTTP_BAD_REQUEST, make_error_body(true))},
        TestCase{.test_name = "error_plain_text_response",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_BAD_REQUEST,
                 .body = "plain text",
                 .expected_response = make_response(
                     net::OK, net::HTTP_BAD_REQUEST, make_error_body(true))},
        TestCase{.test_name = "error_invalid_json_response",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_BAD_REQUEST,
                 .body = R"({"invalid": json})",
                 .expected_response = make_response(
                     net::OK, net::HTTP_BAD_REQUEST, make_error_body(true))},
        TestCase{.test_name = "error_valid_json_lacks_error_field",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_BAD_REQUEST,
                 .body = R"({"valid": "json"})",
                 .expected_response = make_response(
                     net::OK, net::HTTP_BAD_REQUEST, make_error_body(true))},
        TestCase{.test_name = "error_valid_json_has_error_field",
                 .request = make_request(),
                 .net_error = net::OK,
                 .status_code = net::HTTP_BAD_REQUEST,
                 .body = R"({"error": "error"})",
                 .expected_response = make_response(
                     net::OK, net::HTTP_BAD_REQUEST, make_error_body(false))}};
  }

 protected:
  void Interceptor(const network::ResourceRequest& resource_request) {
    const auto& test_case = this->GetParam();

    EXPECT_EQ(resource_request.method, Request::Method());
    EXPECT_EQ(resource_request.url, Endpoint::URL());
    const auto& request_body = CHECK_DEREF(resource_request.request_body);
    const auto& elements = CHECK_DEREF(request_body.elements());
    CHECK_EQ(elements.size(), 1u);
    const auto& element = elements.front();
    EXPECT_EQ(element.type(), network::DataElement::Tag::kBytes);
    const auto body = base::JSONReader::Read(
        element.As<network::DataElementBytes>().AsStringPiece(),
        base::JSON_PARSE_RFC);
    CHECK(body && body->is_dict());
    const auto* request = body->GetDict().FindString(kRequestKey);
    EXPECT_NE(request, nullptr);
    EXPECT_EQ(*request, test_case.request.text);
    EXPECT_EQ(resource_request.headers.GetHeader(
                  net::HttpRequestHeaders::kContentType),
              "application/json");
    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      EXPECT_EQ(resource_request.headers.GetHeader(
                    net::HttpRequestHeaders::kAuthorization),
                "Bearer 12345");
    }

    auto head = test_case.status_code
                    ? network::CreateURLResponseHead(*test_case.status_code)
                    : network::mojom::URLResponseHead::New();
    if constexpr (base::is_instantiation<Response, WithHeaders>) {
      head->headers = test_case.expected_response.headers;
    }
    test_url_loader_factory_.AddResponse(
        resource_request.url, std::move(head), test_case.body,
        network::URLLoaderCompletionStatus(test_case.net_error));
  }

  void RunTestCase() {
    const auto& test_case = this->GetParam();

    test_url_loader_factory_.SetInterceptor(
        base::BindRepeating(&ClientTest::Interceptor, base::Unretained(this)));

    base::test::TestFuture<Response> future;
    Client<Endpoint>::Send(test_url_loader_factory_.GetSafeWeakWrapper(),
                           test_case.request, future.GetCallback());
    EXPECT_EQ(future.Take(), test_case.expected_response);
  }

  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

using ClientTestPlainRequest = ClientTest<TestEndpoint>;

using ClientTestRequestWithHeaders =
    ClientTest<TestEndpoint, WithHeaders<TestEndpoint::Request>>;

using ClientTestResponseWithHeaders =
    ClientTest<TestEndpoint,
               TestEndpoint::Request,
               WithHeaders<TestEndpoint::Response>>;

using ClientTestEmptySuccessBody = ClientTest<TestEndpointWithEmptySuccessBody>;

using ClientTestEmptyErrorBody = ClientTest<TestEndpointWithEmptyErrorBody>;

enum class CancelRequestOn { kSameSequence, kDifferentSequence };

class ClientTestCancelableRequest
    : public testing::TestWithParam<CancelRequestOn> {
 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

}  // namespace

TEST_P(ClientTestPlainRequest, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    PlainRequest,
    ClientTestPlainRequest,
    testing::ValuesIn(ClientTestPlainRequest::GenerateTestCases()),
    ClientTestPlainRequest::kNameGenerator);

TEST_P(ClientTestRequestWithHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithHeaders,
    ClientTestRequestWithHeaders,
    testing::ValuesIn(ClientTestRequestWithHeaders::GenerateTestCases()),
    ClientTestRequestWithHeaders::kNameGenerator);

TEST_P(ClientTestResponseWithHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ResponseWithHeaders,
    ClientTestResponseWithHeaders,
    testing::ValuesIn(ClientTestResponseWithHeaders::GenerateTestCases()),
    ClientTestResponseWithHeaders::kNameGenerator);

TEST_P(ClientTestEmptySuccessBody, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    EmptySuccessBody,
    ClientTestEmptySuccessBody,
    testing::ValuesIn(ClientTestEmptySuccessBody::GenerateTestCases()),
    ClientTestEmptySuccessBody::kNameGenerator);

TEST_P(ClientTestEmptyErrorBody, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    EmptyErrorBody,
    ClientTestEmptyErrorBody,
    testing::ValuesIn(ClientTestEmptyErrorBody::GenerateTestCases()),
    ClientTestEmptyErrorBody::kNameGenerator);

TEST_P(ClientTestCancelableRequest, Cancel) {
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

INSTANTIATE_TEST_SUITE_P(CancelableRequest,
                         ClientTestCancelableRequest,
                         testing::Values(CancelRequestOn::kSameSequence,
                                         CancelRequestOn::kDifferentSequence),
                         [](const auto& info) {
                           return info.param == CancelRequestOn::kSameSequence
                                      ? "same_sequence"
                                      : "different_sequence";
                         });

}  // namespace brave_account::endpoint_client
