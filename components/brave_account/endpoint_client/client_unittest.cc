/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/client.h"

#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/thread.h"
#include "base/types/expected.h"
#include "base/types/is_instantiation.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/is_request.h"
#include "brave/components/brave_account/endpoint_client/is_response.h"
#include "brave/components/brave_account/endpoint_client/is_response_body.h"
#include "brave/components/brave_account/endpoint_client/json_empty_body.h"
#include "brave/components/brave_account/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/brave_account/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/brave_account/endpoint_client/protobuf_empty_body.pb.h"
#include "brave/components/brave_account/endpoint_client/protobuf_test_endpoint_bodies.pb.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
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

bool operator==(const JSONRequestBody& lhs, const JSONRequestBody& rhs) {
  return lhs.request == rhs.request;
}

bool operator==(const JSONSuccessBody& lhs, const JSONSuccessBody& rhs) {
  return lhs.success == rhs.success;
}

bool operator==(const JSONErrorBody& lhs, const JSONErrorBody& rhs) {
  return lhs.error == rhs.error;
}

inline bool operator==(const JSONEmptyBody&, const JSONEmptyBody&) {
  return true;
}

bool operator==(const ProtobufRequestBody& lhs,
                const ProtobufRequestBody& rhs) {
  return lhs.request() == rhs.request();
}

bool operator==(const ProtobufSuccessBody& lhs,
                const ProtobufSuccessBody& rhs) {
  return lhs.success() == rhs.success();
}

bool operator==(const ProtobufErrorBody& lhs, const ProtobufErrorBody& rhs) {
  return lhs.error() == rhs.error();
}

bool operator==(const ProtobufEmptyBody&, const ProtobufEmptyBody&) {
  return true;
}

namespace {

template <detail::IsRequest RequestT, detail::IsResponse ResponseT>
struct TestEndpoint {
  using Request = RequestT;
  using Response = ResponseT;
  static GURL URL() { return GURL("https://example.com/api/query"); }
};

// Request: POST https://example.com/api/query { "request": "abc" }
// Success: { "success": "ok" }
// Error:   { "error": "nope" }
using JSONEndpointSuccessError =
    TestEndpoint<POST<JSONRequestBody>,
                 Response<JSONSuccessBody, JSONErrorBody>>;

// Request: POST https://example.com/api/query { "request": "abc" }
// Success: ignored (response body not parsed)
// Error:   { "error": "nope" }
using JSONEndpointEmptyError =
    TestEndpoint<POST<JSONRequestBody>, Response<JSONEmptyBody, JSONErrorBody>>;

// Request: POST https://example.com/api/query { "request": "abc" }
// Success: { "success": "ok" }
// Error:   ignored (response body not parsed)
using JSONEndpointSuccessEmpty =
    TestEndpoint<POST<JSONRequestBody>,
                 Response<JSONSuccessBody, JSONEmptyBody>>;

// Request: POST https://example.com/api/query \x0A\x03abc
// Success: \x0A\x02ok
// Error:   \x0A\x04nope
using ProtobufEndpointSuccessError =
    TestEndpoint<POST<ProtobufRequestBody>,
                 Response<ProtobufSuccessBody, ProtobufErrorBody>>;

// Request: POST https://example.com/api/query \x0A\x03abc
// Success: ignored (response body not parsed)
// Error:   \x0A\x04nope
using ProtobufEndpointEmptyError =
    TestEndpoint<POST<ProtobufRequestBody>,
                 Response<ProtobufEmptyBody, ProtobufErrorBody>>;

// Request: POST https://example.com/api/query \x0A\x03abc
// Success: \x0A\x02ok
// Error:   ignored (response body not parsed)
using ProtobufEndpointSuccessEmpty =
    TestEndpoint<POST<ProtobufRequestBody>,
                 Response<ProtobufSuccessBody, ProtobufEmptyBody>>;

template <typename Response>
  requires detail::IsResponse<detail::MaybeStripWithHeaders<Response>>
struct TestCase {
  std::string test_name;
  int net_error;
  std::optional<net::HttpStatusCode> status_code;
  std::string body;
  Response response;
};

template <IsEndpoint Endpoint, typename Request, typename Response>
  requires(std::same_as<detail::MaybeStripWithHeaders<Request>,
                        typename Endpoint::Request> &&
           std::same_as<detail::MaybeStripWithHeaders<Response>,
                        typename Endpoint::Response>)
class ClientTest : public testing::TestWithParam<const TestCase<Response>*> {
 public:
  static constexpr auto kNameGenerator = [](const auto& info) {
    return CHECK_DEREF(info.param).test_name;
  };

  static auto GenerateTestCases() {
    return testing::Values(NetError(), NoResponseHeaders(),
                           SuccessEmptyResponse(), SuccessMalformedResponse(),
                           SuccessUnexpectedResponse(), SuccessValidResponse(),
                           ErrorEmptyResponse(), ErrorMalformedResponse(),
                           ErrorUnexpectedResponse(), ErrorValidResponse());
  }

 protected:
  void RunTestCase() {
    const auto& test_case = CHECK_DEREF(this->GetParam());

    test_url_loader_factory_.SetInterceptor(
        base::BindRepeating(&ClientTest::Interceptor, base::Unretained(this)));

    base::test::TestFuture<Response> future;
    Client<Endpoint>::Send(test_url_loader_factory_.GetSafeWeakWrapper(),
                           MakeRequest(), future.GetCallback());
    EXPECT_EQ(future.Take(), test_case.response);
  }

 private:
  using TestCase = TestCase<Response>;

  enum class BodyCase { kMalformed, kUnexpected, kValid };

  static const TestCase* NetError() {
    static const base::NoDestructor<TestCase> kNetError({
        .test_name = "net_error",
        .net_error = net::ERR_FAILED,
        .status_code = std::nullopt,
        .body = "",
        .response = MakeResponse(net::ERR_FAILED, std::nullopt, std::nullopt),
    });
    return kNetError.get();
  }

  static const TestCase* NoResponseHeaders() {
    static const base::NoDestructor<TestCase> kNoResponseHeaders({
        .test_name = "no_response_headers",
        .net_error = net::OK,
        .status_code = std::nullopt,
        .body = "",
        .response = MakeResponse(net::OK, std::nullopt, std::nullopt),
    });
    return kNoResponseHeaders.get();
  }

  static const TestCase* SuccessEmptyResponse() {
    static const base::NoDestructor<TestCase> kSuccessEmptyResponse({
        .test_name = "success_empty_response",
        .net_error = net::OK,
        .status_code = net::HTTP_OK,
        .body = "",
        .response = MakeResponse(net::OK, net::HTTP_OK, std::nullopt),
    });
    return kSuccessEmptyResponse.get();
  }

  static const TestCase* SuccessMalformedResponse() {
    static const base::NoDestructor<TestCase> kSuccessMalformedResponse({
        .test_name = "success_malformed_response",
        .net_error = net::OK,
        .status_code = net::HTTP_OK,
        .body = MakeBody<typename Endpoint::Response::SuccessBody>(
            BodyCase::kMalformed),
        .response = MakeResponse(net::OK, net::HTTP_OK, BodyCase::kMalformed),
    });
    return kSuccessMalformedResponse.get();
  }

  static const TestCase* SuccessUnexpectedResponse() {
    static const base::NoDestructor<TestCase> kSuccessUnexpectedResponse({
        .test_name = "success_unexpected_response",
        .net_error = net::OK,
        .status_code = net::HTTP_OK,
        .body = MakeBody<typename Endpoint::Response::SuccessBody>(
            BodyCase::kUnexpected),
        .response = MakeResponse(net::OK, net::HTTP_OK, BodyCase::kUnexpected),
    });
    return kSuccessUnexpectedResponse.get();
  }

  static const TestCase* SuccessValidResponse() {
    static const base::NoDestructor<TestCase> kSuccessValidResponse({
        .test_name = "success_valid_response",
        .net_error = net::OK,
        .status_code = net::HTTP_OK,
        .body = MakeBody<typename Endpoint::Response::SuccessBody>(
            BodyCase::kValid),
        .response = MakeResponse(net::OK, net::HTTP_OK, BodyCase::kValid),
    });
    return kSuccessValidResponse.get();
  }

  static const TestCase* ErrorEmptyResponse() {
    static const base::NoDestructor<TestCase> kErrorEmptyResponse({
        .test_name = "error_empty_response",
        .net_error = net::OK,
        .status_code = net::HTTP_BAD_REQUEST,
        .body = "",
        .response = MakeResponse(net::OK, net::HTTP_BAD_REQUEST, std::nullopt),
    });
    return kErrorEmptyResponse.get();
  }

  static const TestCase* ErrorMalformedResponse() {
    static const base::NoDestructor<TestCase> kErrorMalformedResponse({
        .test_name = "error_malformed_response",
        .net_error = net::OK,
        .status_code = net::HTTP_BAD_REQUEST,
        .body = MakeBody<typename Endpoint::Response::ErrorBody>(
            BodyCase::kMalformed),
        .response =
            MakeResponse(net::OK, net::HTTP_BAD_REQUEST, BodyCase::kMalformed),
    });
    return kErrorMalformedResponse.get();
  }

  static const TestCase* ErrorUnexpectedResponse() {
    static const base::NoDestructor<TestCase> kErrorUnexpectedResponse({
        .test_name = "error_unexpected_response",
        .net_error = net::OK,
        .status_code = net::HTTP_BAD_REQUEST,
        .body = MakeBody<typename Endpoint::Response::ErrorBody>(
            BodyCase::kUnexpected),
        .response =
            MakeResponse(net::OK, net::HTTP_BAD_REQUEST, BodyCase::kUnexpected),
    });
    return kErrorUnexpectedResponse.get();
  }

  static const TestCase* ErrorValidResponse() {
    static const base::NoDestructor<TestCase> kErrorValidResponse({
        .test_name = "error_valid_response",
        .net_error = net::OK,
        .status_code = net::HTTP_BAD_REQUEST,
        .body =
            MakeBody<typename Endpoint::Response::ErrorBody>(BodyCase::kValid),
        .response =
            MakeResponse(net::OK, net::HTTP_BAD_REQUEST, BodyCase::kValid),
    });
    return kErrorValidResponse.get();
  }

  template <std::same_as<JSONRequestBody> RequestBody>
  static RequestBody MakeRequestBody() {
    RequestBody request_body;
    request_body.request = "request";
    return request_body;
  }

  template <std::same_as<ProtobufRequestBody> RequestBody>
  static RequestBody MakeRequestBody() {
    RequestBody request_body;
    request_body.set_request("request");
    return request_body;
  }

  static Request MakeRequest() {
    Request request;
    request.body = MakeRequestBody<typename Endpoint::Request::Body>();

    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      SetBearerToken(request, "12345");
    }

    return request;
  }

  template <detail::IsJSONResponseBody ResponseBody>
  static auto MakeResponseBody(std::optional<BodyCase> body_case) {
    std::optional<ResponseBody> response_body;

    if constexpr (std::same_as<ResponseBody, JSONEmptyBody>) {
      response_body.emplace();
    } else if (body_case) {
      switch (*body_case) {
        case BodyCase::kMalformed:
          break;
        case BodyCase::kUnexpected:
          break;
        case BodyCase::kValid:
          if constexpr (std::same_as<ResponseBody, JSONSuccessBody>) {
            response_body.emplace().success = "success";
          } else {
            static_assert(std::same_as<ResponseBody, JSONErrorBody>);
            response_body.emplace().error = "error";
          }
      }
    }

    return response_body;
  }

  template <detail::IsProtobufResponseBody ResponseBody>
  static auto MakeResponseBody(std::optional<BodyCase> body_case) {
    std::optional<ResponseBody> response_body;

    if constexpr (std::same_as<ResponseBody, ProtobufEmptyBody>) {
      response_body.emplace();
    } else if (body_case) {
      switch (*body_case) {
        case BodyCase::kMalformed:
          break;
        case BodyCase::kUnexpected:
          // Unexpected payloads are well-formed protobuf wire formats that do
          // not match the expected ResponseBody schema (e.g. contain only
          // unknown fields). Parsing succeeds, but all fields are ignored,
          // yielding a default-constructed ResponseBody. See
          // Response<>::Deserialize<>() for details on protobuf parsing
          // semantics.
          response_body.emplace();
          break;
        case BodyCase::kValid:
          if constexpr (std::same_as<ResponseBody, ProtobufSuccessBody>) {
            response_body.emplace().set_success("success");
          } else {
            static_assert(std::same_as<ResponseBody, ProtobufErrorBody>);
            response_body.emplace().set_error("error");
          }
      }
    }

    return response_body;
  }

  static Response MakeResponse(int net_error,
                               std::optional<net::HttpStatusCode> status_code,
                               std::optional<BodyCase> body_case) {
    Response response;
    response.net_error = net_error;
    response.status_code = status_code;

    if (net_error == net::OK && status_code) {
      if (network::IsSuccessfulStatus(*status_code)) {  // 2xx
        response.body =
            MakeResponseBody<typename Endpoint::Response::SuccessBody>(
                body_case);
      } else {  // non-2xx
        response.body =
            MakeResponseBody<typename Endpoint::Response::ErrorBody>(body_case)
                .transform([](auto error_body) {
                  return base::unexpected(std::move(error_body));
                });
      }

      if constexpr (base::is_instantiation<Response, WithHeaders>) {
        response.headers =
            net::HttpResponseHeaders::Builder(
                {1, 1}, absl::StrFormat("%d %s", *status_code,
                                        net::GetHttpReasonPhrase(*status_code)))
                .AddHeader("X-Test-Header", "test-value")
                .Build();
      }
    }

    return response;
  }

  template <detail::IsJSONResponseBody ResponseBody>
  static std::string MakeBody(BodyCase body_case) {
    switch (body_case) {
      case BodyCase::kMalformed:
        return R"({"invalid": json})";
      case BodyCase::kUnexpected:
        return R"({"valid": "json"})";
      case BodyCase::kValid: {
        ResponseBody response_body;

        if constexpr (!std::same_as<ResponseBody, JSONEmptyBody>) {
          if constexpr (std::same_as<ResponseBody, JSONSuccessBody>) {
            response_body.success = "success";
          } else {
            static_assert(std::same_as<ResponseBody, JSONErrorBody>);
            response_body.error = "error";
          }
        }

        return CHECK_DEREF(base::WriteJson(response_body.ToValue()));
      }
    }
  }

  template <detail::IsProtobufResponseBody ResponseBody>
  static std::string MakeBody(BodyCase body_case) {
    switch (body_case) {
      case BodyCase::kMalformed:
        // Field 1, length=10, but only 8 bytes follow ("tooshort").
        return "\x0A\x0Atooshort";
      case BodyCase::kUnexpected:
        // Field 2, length=10, value="unexpected".
        // Well-formed protobuf payload containing only unknown fields.
        // ParseFromString() succeeds, yielding a default-constructed
        // ResponseBody. See Response<>::Deserialize<>() for details on protobuf
        // parsing semantics.
        return "\x12\x0Aunexpected";
      case BodyCase::kValid: {
        ResponseBody response_body;

        if constexpr (!std::same_as<ResponseBody, ProtobufEmptyBody>) {
          if constexpr (std::same_as<ResponseBody, ProtobufSuccessBody>) {
            response_body.set_success("success");
          } else {
            static_assert(std::same_as<ResponseBody, ProtobufErrorBody>);
            response_body.set_error("error");
          }
        }

        return response_body.SerializeAsString();
      }
    }
  }

  template <std::same_as<JSONRequestBody> RequestBody>
  RequestBody RequestBodyFrom(std::string_view bytes) {
    return CHECK_DEREF(RequestBody::FromValue(
        CHECK_DEREF(base::JSONReader::Read(bytes, base::JSON_PARSE_RFC))));
  }

  template <std::same_as<ProtobufRequestBody> RequestBody>
  RequestBody RequestBodyFrom(std::string_view bytes) {
    RequestBody body;
    CHECK(body.ParseFromString(bytes));
    return body;
  }

  void Interceptor(const network::ResourceRequest& resource_request) {
    EXPECT_EQ(resource_request.url, Endpoint::URL());
    EXPECT_EQ(resource_request.method, Request::Method());
    EXPECT_EQ(resource_request.headers.GetHeader(
                  net::HttpRequestHeaders::kContentType),
              Request::ContentType());

    const auto& request_body = CHECK_DEREF(resource_request.request_body);
    const auto& elements = CHECK_DEREF(request_body.elements());
    CHECK_EQ(elements.size(), 1u);
    const auto& element = elements.front();
    EXPECT_EQ(element.type(), network::DataElement::Tag::kBytes);
    const auto bytes = element.As<network::DataElementBytes>().AsStringPiece();
    EXPECT_EQ(RequestBodyFrom<typename Endpoint::Request::Body>(bytes),
              MakeRequest().body);

    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      EXPECT_EQ(resource_request.headers.GetHeader(
                    net::HttpRequestHeaders::kAuthorization),
                "Bearer 12345");
    }

    const auto& test_case = CHECK_DEREF(this->GetParam());
    auto head = test_case.status_code
                    ? network::CreateURLResponseHead(*test_case.status_code)
                    : network::mojom::URLResponseHead::New();
    if constexpr (base::is_instantiation<Response, WithHeaders>) {
      head->headers = test_case.response.headers;
    }

    test_url_loader_factory_.AddResponse(
        resource_request.url, std::move(head), test_case.body,
        network::URLLoaderCompletionStatus(test_case.net_error));
  }

  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

using ClientTestJSONPlainRequest =
    ClientTest<JSONEndpointSuccessError,
               JSONEndpointSuccessError::Request,
               JSONEndpointSuccessError::Response>;

using ClientTestJSONRequestWithHeaders =
    ClientTest<JSONEndpointSuccessError,
               WithHeaders<JSONEndpointSuccessError::Request>,
               JSONEndpointSuccessError::Response>;

using ClientTestJSONResponseWithHeaders =
    ClientTest<JSONEndpointSuccessError,
               JSONEndpointSuccessError::Request,
               WithHeaders<JSONEndpointSuccessError::Response>>;

using ClientTestJSONEmptySuccessBody =
    ClientTest<JSONEndpointEmptyError,
               JSONEndpointEmptyError::Request,
               JSONEndpointEmptyError::Response>;

using ClientTestJSONEmptyErrorBody =
    ClientTest<JSONEndpointSuccessEmpty,
               JSONEndpointSuccessEmpty::Request,
               JSONEndpointSuccessEmpty::Response>;

using ClientTestProtobufPlainRequest =
    ClientTest<ProtobufEndpointSuccessError,
               ProtobufEndpointSuccessError::Request,
               ProtobufEndpointSuccessError::Response>;

using ClientTestProtobufRequestWithHeaders =
    ClientTest<ProtobufEndpointSuccessError,
               WithHeaders<ProtobufEndpointSuccessError::Request>,
               ProtobufEndpointSuccessError::Response>;

using ClientTestProtobufResponseWithHeaders =
    ClientTest<ProtobufEndpointSuccessError,
               ProtobufEndpointSuccessError::Request,
               WithHeaders<ProtobufEndpointSuccessError::Response>>;

using ClientTestProtobufEmptySuccessBody =
    ClientTest<ProtobufEndpointEmptyError,
               ProtobufEndpointEmptyError::Request,
               ProtobufEndpointEmptyError::Response>;

using ClientTestProtobufEmptyErrorBody =
    ClientTest<ProtobufEndpointSuccessEmpty,
               ProtobufEndpointSuccessEmpty::Request,
               ProtobufEndpointSuccessEmpty::Response>;

enum class CancelRequestOn { kSameSequence, kDifferentSequence };

class ClientTestCancelableRequest
    : public testing::TestWithParam<CancelRequestOn> {
 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

}  // namespace

TEST_P(ClientTestJSONPlainRequest, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(JSONPlainRequest,
                         ClientTestJSONPlainRequest,
                         ClientTestJSONPlainRequest::GenerateTestCases(),
                         ClientTestJSONPlainRequest::kNameGenerator);

TEST_P(ClientTestJSONRequestWithHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(JSONRequestWithHeaders,
                         ClientTestJSONRequestWithHeaders,
                         ClientTestJSONRequestWithHeaders::GenerateTestCases(),
                         ClientTestJSONRequestWithHeaders::kNameGenerator);

TEST_P(ClientTestJSONResponseWithHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(JSONResponseWithHeaders,
                         ClientTestJSONResponseWithHeaders,
                         ClientTestJSONResponseWithHeaders::GenerateTestCases(),
                         ClientTestJSONResponseWithHeaders::kNameGenerator);

TEST_P(ClientTestJSONEmptySuccessBody, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(JSONEmptySuccessBody,
                         ClientTestJSONEmptySuccessBody,
                         ClientTestJSONEmptySuccessBody::GenerateTestCases(),
                         ClientTestJSONEmptySuccessBody::kNameGenerator);

TEST_P(ClientTestJSONEmptyErrorBody, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(JSONEmptyErrorBody,
                         ClientTestJSONEmptyErrorBody,
                         ClientTestJSONEmptyErrorBody::GenerateTestCases(),
                         ClientTestJSONEmptyErrorBody::kNameGenerator);

TEST_P(ClientTestProtobufPlainRequest, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(ProtobufPlainRequest,
                         ClientTestProtobufPlainRequest,
                         ClientTestProtobufPlainRequest::GenerateTestCases(),
                         ClientTestProtobufPlainRequest::kNameGenerator);

TEST_P(ClientTestProtobufRequestWithHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufRequestWithHeaders,
    ClientTestProtobufRequestWithHeaders,
    ClientTestProtobufRequestWithHeaders::GenerateTestCases(),
    ClientTestProtobufRequestWithHeaders::kNameGenerator);

TEST_P(ClientTestProtobufResponseWithHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufResponseWithHeaders,
    ClientTestProtobufResponseWithHeaders,
    ClientTestProtobufResponseWithHeaders::GenerateTestCases(),
    ClientTestProtobufResponseWithHeaders::kNameGenerator);

TEST_P(ClientTestProtobufEmptySuccessBody, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufEmptySuccessBody,
    ClientTestProtobufEmptySuccessBody,
    ClientTestProtobufEmptySuccessBody::GenerateTestCases(),
    ClientTestProtobufEmptySuccessBody::kNameGenerator);

TEST_P(ClientTestProtobufEmptyErrorBody, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(ProtobufEmptyErrorBody,
                         ClientTestProtobufEmptyErrorBody,
                         ClientTestProtobufEmptyErrorBody::GenerateTestCases(),
                         ClientTestProtobufEmptyErrorBody::kNameGenerator);

TEST_P(ClientTestCancelableRequest, Cancel) {
  base::test::TestFuture<JSONEndpointSuccessError::Response> future;
  RequestHandle request_handle =
      Client<JSONEndpointSuccessError>::Send<RequestCancelability::kCancelable>(
          test_url_loader_factory_.GetSafeWeakWrapper(),
          JSONEndpointSuccessError::Request(), future.GetCallback());

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
