/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/client.h"

#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/no_destructor.h"
#include "base/test/bind.h"
#include "base/test/gtest_util.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/thread.h"
#include "base/types/expected.h"
#include "base/types/is_instantiation.h"
#include "brave/components/endpoint_client/is_endpoint.h"
#include "brave/components/endpoint_client/is_request.h"
#include "brave/components/endpoint_client/is_request_body.h"
#include "brave/components/endpoint_client/is_response.h"
#include "brave/components/endpoint_client/is_response_body.h"
#include "brave/components/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/endpoint_client/protobuf_test_endpoint_bodies.pb.h"
#include "brave/components/endpoint_client/request_handle.h"
#include "brave/components/endpoint_client/request_types.h"
#include "brave/components/endpoint_client/response.h"
#include "brave/components/endpoint_client/retry_options.h"
#include "brave/components/endpoint_client/test_support.h"
#include "brave/components/endpoint_client/url_replacements.h"
#include "brave/components/endpoint_client/with_headers.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

namespace endpoint_client {

namespace {

template <internal::IsRequest RequestT, internal::IsResponse ResponseT>
struct TestEndpoint {
  using Request = RequestT;
  using Response = ResponseT;
  static GURL URL() { return GURL("https://example.com/api/query"); }
};

// Request: POST https://example.com/api/query { "request": "request" }
// Success: { "success": "success" }
// Error:   { "error": "error" }
using JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint =
    TestEndpoint<Post<JSONNonEmptyRequestBody>,
                 Response<JSONSuccessBody, JSONErrorBody>>;

// Request: POST https://example.com/api/query {}
// Success: { "success": "success" }
// Error:   { "error": "error" }
using JSONEmptyRequestBodyJSONResponseBodiesEndpoint =
    TestEndpoint<Post<JSONEmptyRequestBody>,
                 Response<JSONSuccessBody, JSONErrorBody>>;

// Request: POST https://example.com/api/query (no body)
// Success: { "success": "success" }
// Error:   { "error": "error" }
using RequestWithoutBodyJSONResponseBodiesEndpoint =
    TestEndpoint<Post<>, Response<JSONSuccessBody, JSONErrorBody>>;

// Request: POST https://example.com/api/query \x0A\x07request
// Success: \x0A\x07success
// Error:   \x0A\x05error
using ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint =
    TestEndpoint<Post<ProtobufNonEmptyRequestBody>,
                 Response<ProtobufSuccessBody, ProtobufErrorBody>>;

// Request: POST https://example.com/api/query (zero bytes)
// Success: \x0A\x07success
// Error:   \x0A\x05error
using ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint =
    TestEndpoint<Post<ProtobufEmptyRequestBody>,
                 Response<ProtobufSuccessBody, ProtobufErrorBody>>;

// Request: POST https://example.com/api/query (no body)
// Success: \x0A\x07success
// Error:   \x0A\x05error
using RequestWithoutBodyProtobufResponseBodiesEndpoint =
    TestEndpoint<Post<>, Response<ProtobufSuccessBody, ProtobufErrorBody>>;

// Request: POST https://example.com/api/query { "request": "request" }
// Success: \x0A\x07success
// Error:   \x0A\x05error
using JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint =
    TestEndpoint<Post<JSONNonEmptyRequestBody>,
                 Response<ProtobufSuccessBody, ProtobufErrorBody>>;

// Request: POST https://example.com/api/query \x0A\x07request
// Success: { "success": "success" }
// Error:   { "error": "error" }
using ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint =
    TestEndpoint<Post<ProtobufNonEmptyRequestBody>,
                 Response<JSONSuccessBody, JSONErrorBody>>;

// Request: POST https://example.com/api/query (no body)
// Success: \x0A\x07success
// Error:   { "error": "error" }
using RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint =
    TestEndpoint<Post<>, Response<ProtobufSuccessBody, JSONErrorBody>>;

// Request: POST https://example.com/api/query (no body)
// Success: not parsed
// Error:   not parsed
using RequestWithoutBodyResponseWithoutBodiesEndpoint =
    TestEndpoint<Post<>, Response<>>;

template <typename Response>
  requires internal::IsResponse<internal::MaybeStripWithHeaders<Response>>
struct TestCase {
  std::string test_name;
  int net_error;
  std::optional<net::HttpStatusCode> status_code;
  std::string body;
  Response response;
};

template <IsEndpoint Endpoint, typename Request, typename Response>
  requires(std::same_as<internal::MaybeStripWithHeaders<Request>,
                        typename Endpoint::Request> &&
           std::same_as<internal::MaybeStripWithHeaders<Response>,
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
    Client<Endpoint>::Send(test_url_loader_factory_, MakeRequest(),
                           future.GetCallback());
    EXPECT_THAT(future.Take(), ResponseEq(test_case.response));
  }

 private:
  using TestCase = TestCase<Response>;

  static constexpr bool kIsRequestWithoutBody =
      internal::IsRequestWithoutBody<typename Endpoint::Request>;

  static constexpr bool kIsResponseWithoutBodies =
      internal::IsResponseWithoutBodies<typename Endpoint::Response>;

  enum class BodyCase { kEmpty, kMalformed, kUnexpected, kValid };

  static const TestCase* NetError() {
    static const base::NoDestructor<TestCase> kNetError({
        .test_name = "net_error",
        .net_error = net::ERR_FAILED,
        .status_code = std::nullopt,
        .body = "",
        .response =
            MakeResponse(net::ERR_FAILED, std::nullopt, BodyCase::kEmpty),
    });
    return kNetError.get();
  }

  static const TestCase* NoResponseHeaders() {
    static const base::NoDestructor<TestCase> kNoResponseHeaders({
        .test_name = "no_response_headers",
        .net_error = net::OK,
        .status_code = std::nullopt,
        .body = "",
        .response = MakeResponse(net::OK, std::nullopt, BodyCase::kEmpty),
    });
    return kNoResponseHeaders.get();
  }

  static const TestCase* SuccessEmptyResponse() {
    static const base::NoDestructor<TestCase> kSuccessEmptyResponse({
        .test_name = "success_empty_response",
        .net_error = net::OK,
        .status_code = net::HTTP_OK,
        .body = MakeSuccessBody(BodyCase::kEmpty),
        .response = MakeResponse(net::OK, net::HTTP_OK, BodyCase::kEmpty),
    });
    return kSuccessEmptyResponse.get();
  }

  static const TestCase* SuccessMalformedResponse() {
    static const base::NoDestructor<TestCase> kSuccessMalformedResponse({
        .test_name = "success_malformed_response",
        .net_error = net::OK,
        .status_code = net::HTTP_OK,
        .body = MakeSuccessBody(BodyCase::kMalformed),
        .response = MakeResponse(net::OK, net::HTTP_OK, BodyCase::kMalformed),
    });
    return kSuccessMalformedResponse.get();
  }

  static const TestCase* SuccessUnexpectedResponse() {
    static const base::NoDestructor<TestCase> kSuccessUnexpectedResponse({
        .test_name = "success_unexpected_response",
        .net_error = net::OK,
        .status_code = net::HTTP_OK,
        .body = MakeSuccessBody(BodyCase::kUnexpected),
        .response = MakeResponse(net::OK, net::HTTP_OK, BodyCase::kUnexpected),
    });
    return kSuccessUnexpectedResponse.get();
  }

  static const TestCase* SuccessValidResponse() {
    static const base::NoDestructor<TestCase> kSuccessValidResponse({
        .test_name = "success_valid_response",
        .net_error = net::OK,
        .status_code = net::HTTP_OK,
        .body = MakeSuccessBody(BodyCase::kValid),
        .response = MakeResponse(net::OK, net::HTTP_OK, BodyCase::kValid),
    });
    return kSuccessValidResponse.get();
  }

  static const TestCase* ErrorEmptyResponse() {
    static const base::NoDestructor<TestCase> kErrorEmptyResponse({
        .test_name = "error_empty_response",
        .net_error = net::OK,
        .status_code = net::HTTP_BAD_REQUEST,
        .body = MakeErrorBody(BodyCase::kEmpty),
        .response =
            MakeResponse(net::OK, net::HTTP_BAD_REQUEST, BodyCase::kEmpty),
    });
    return kErrorEmptyResponse.get();
  }

  static const TestCase* ErrorMalformedResponse() {
    static const base::NoDestructor<TestCase> kErrorMalformedResponse({
        .test_name = "error_malformed_response",
        .net_error = net::OK,
        .status_code = net::HTTP_BAD_REQUEST,
        .body = MakeErrorBody(BodyCase::kMalformed),
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
        .body = MakeErrorBody(BodyCase::kUnexpected),
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
        .body = MakeErrorBody(BodyCase::kValid),
        .response =
            MakeResponse(net::OK, net::HTTP_BAD_REQUEST, BodyCase::kValid),
    });
    return kErrorValidResponse.get();
  }

  template <std::same_as<JSONNonEmptyRequestBody> RequestBody>
  static RequestBody MakeRequestBody() {
    RequestBody request_body;
    request_body.request = "request";
    return request_body;
  }

  template <std::same_as<JSONEmptyRequestBody> RequestBody>
  static RequestBody MakeRequestBody() {
    return RequestBody();
  }

  template <std::same_as<ProtobufNonEmptyRequestBody> RequestBody>
  static RequestBody MakeRequestBody() {
    RequestBody request_body;
    request_body.set_request("request");
    return request_body;
  }

  template <std::same_as<ProtobufEmptyRequestBody> RequestBody>
  static RequestBody MakeRequestBody() {
    return RequestBody();
  }

  static Request MakeRequest() {
    Request request;
    if constexpr (!kIsRequestWithoutBody) {
      request.body = MakeRequestBody<typename Endpoint::Request::Body>();
    }

    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      SetBearerToken(request, "12345");
    }

    return request;
  }

  template <internal::IsJSONResponseBody ResponseBody>
  static auto MakeResponseBody(BodyCase body_case) {
    std::optional<ResponseBody> response_body;

    switch (body_case) {
      case BodyCase::kEmpty:
        break;
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

    return response_body;
  }

  template <internal::IsProtobufResponseBody ResponseBody>
  static auto MakeResponseBody(BodyCase body_case) {
    std::optional<ResponseBody> response_body;

    switch (body_case) {
      case BodyCase::kEmpty:
        // Empty string is valid Protobuf wire format (zero-length message),
        // representing a message with no fields set.
        response_body.emplace();
        break;
      case BodyCase::kMalformed:
        break;
      case BodyCase::kUnexpected:
        // Unexpected payloads are well-formed protobuf wire formats that do
        // not match the expected ResponseBody schema (e.g. contain only
        // unknown fields). Parsing succeeds, but all fields are ignored,
        // yielding a default-constructed ResponseBody.
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

    return response_body;
  }

  static Response MakeResponse(int net_error,
                               std::optional<net::HttpStatusCode> status_code,
                               BodyCase body_case) {
    Response response;
    response.net_error = net_error;
    response.status_code = status_code;

    if (net_error == net::OK && status_code) {
      if constexpr (!kIsResponseWithoutBodies) {
        if (network::IsSuccessfulStatus(*status_code)) {  // 2xx
          response.body =
              MakeResponseBody<typename Endpoint::Response::SuccessBody>(
                  body_case);
        } else {  // non-2xx
          response.body =
              MakeResponseBody<typename Endpoint::Response::ErrorBody>(
                  body_case)
                  .transform([](auto error_body) {
                    return base::unexpected(std::move(error_body));
                  });
        }
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

  // The response body served for endpoints whose Response<> has no bodies. Its
  // content doesn't matter: such responses never parse a body, so any bytes
  // must be ignored, and the tests check that only the status reaches the
  // response.
  static constexpr char kIgnoredBody[] = "ignored";

  static std::string MakeSuccessBody(BodyCase body_case) {
    if constexpr (kIsResponseWithoutBodies) {
      return kIgnoredBody;
    } else {
      return MakeBody<typename Endpoint::Response::SuccessBody>(body_case);
    }
  }

  static std::string MakeErrorBody(BodyCase body_case) {
    if constexpr (kIsResponseWithoutBodies) {
      return kIgnoredBody;
    } else {
      return MakeBody<typename Endpoint::Response::ErrorBody>(body_case);
    }
  }

  template <internal::IsJSONResponseBody ResponseBody>
  static std::string MakeBody(BodyCase body_case) {
    switch (body_case) {
      case BodyCase::kEmpty:
        return "";
      case BodyCase::kMalformed:
        return R"({"invalid": json})";
      case BodyCase::kUnexpected:
        return R"({"valid": "json"})";
      case BodyCase::kValid: {
        ResponseBody response_body;

        if constexpr (std::same_as<ResponseBody, JSONSuccessBody>) {
          response_body.success = "success";
        } else {
          static_assert(std::same_as<ResponseBody, JSONErrorBody>);
          response_body.error = "error";
        }

        return CHECK_DEREF(base::WriteJson(response_body.ToValue()));
      }
    }
  }

  template <internal::IsProtobufResponseBody ResponseBody>
  static std::string MakeBody(BodyCase body_case) {
    switch (body_case) {
      case BodyCase::kEmpty:
        return "";
      case BodyCase::kMalformed:
        // Field 1, length=10, but only 8 bytes follow ("tooshort").
        return "\x0A\x0Atooshort";
      case BodyCase::kUnexpected:
        // Field 2, length=10, value="unexpected".
        // Well-formed protobuf payload containing only unknown fields.
        // ParseFromString() succeeds, yielding a default-constructed
        // ResponseBody.
        return "\x12\x0Aunexpected";
      case BodyCase::kValid: {
        ResponseBody response_body;

        if constexpr (std::same_as<ResponseBody, ProtobufSuccessBody>) {
          response_body.set_success("success");
        } else {
          static_assert(std::same_as<ResponseBody, ProtobufErrorBody>);
          response_body.set_error("error");
        }

        return response_body.SerializeAsString();
      }
    }
  }

  template <internal::IsJSONRequestBody RequestBody>
  RequestBody RequestBodyFrom(std::string_view bytes) {
    return CHECK_DEREF(RequestBody::FromValue(
        CHECK_DEREF(base::JSONReader::Read(bytes, base::JSON_PARSE_RFC))));
  }

  template <internal::IsProtobufRequestBody RequestBody>
  RequestBody RequestBodyFrom(std::string_view bytes) {
    RequestBody body;
    CHECK(body.ParseFromString(bytes));
    return body;
  }

  void Interceptor(const network::ResourceRequest& resource_request) {
    EXPECT_EQ(resource_request.url, Endpoint::URL());
    EXPECT_EQ(resource_request.method, Request::Method());
    EXPECT_FALSE(resource_request.SavesCookies());
    EXPECT_EQ(resource_request.redirect_mode,
              network::mojom::RedirectMode::kError);

    if constexpr (kIsRequestWithoutBody) {
      EXPECT_FALSE(resource_request.headers.HasHeader(
          net::HttpRequestHeaders::kContentType));
      EXPECT_FALSE(resource_request.request_body);
    } else {
      EXPECT_EQ(resource_request.headers.GetHeader(
                    net::HttpRequestHeaders::kContentType),
                Request::ContentType());
      EXPECT_TRUE(resource_request.request_body);
      EXPECT_THAT(RequestBodyFrom<typename Endpoint::Request::Body>(
                      network::GetUploadData(resource_request)),
                  internal::BodyEq(MakeRequest().body));
    }

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

using ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesNoHeaders =
    ClientTest<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint,
               JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request,
               JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>;

using ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders =
    ClientTest<
        JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint,
        WithHeaders<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request>,
        JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>;

using ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders =
    ClientTest<
        JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint,
        JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request,
        WithHeaders<
            JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>>;

using ClientTestJSONEmptyRequestBodyJSONResponseBodiesNoHeaders =
    ClientTest<JSONEmptyRequestBodyJSONResponseBodiesEndpoint,
               JSONEmptyRequestBodyJSONResponseBodiesEndpoint::Request,
               JSONEmptyRequestBodyJSONResponseBodiesEndpoint::Response>;

using ClientTestJSONEmptyRequestBodyJSONResponseBodiesRequestWithHeaders =
    ClientTest<
        JSONEmptyRequestBodyJSONResponseBodiesEndpoint,
        WithHeaders<JSONEmptyRequestBodyJSONResponseBodiesEndpoint::Request>,
        JSONEmptyRequestBodyJSONResponseBodiesEndpoint::Response>;

using ClientTestJSONEmptyRequestBodyJSONResponseBodiesResponseWithHeaders =
    ClientTest<
        JSONEmptyRequestBodyJSONResponseBodiesEndpoint,
        JSONEmptyRequestBodyJSONResponseBodiesEndpoint::Request,
        WithHeaders<JSONEmptyRequestBodyJSONResponseBodiesEndpoint::Response>>;

using ClientTestRequestWithoutBodyJSONResponseBodiesNoHeaders =
    ClientTest<RequestWithoutBodyJSONResponseBodiesEndpoint,
               RequestWithoutBodyJSONResponseBodiesEndpoint::Request,
               RequestWithoutBodyJSONResponseBodiesEndpoint::Response>;

using ClientTestRequestWithoutBodyJSONResponseBodiesRequestWithHeaders =
    ClientTest<
        RequestWithoutBodyJSONResponseBodiesEndpoint,
        WithHeaders<RequestWithoutBodyJSONResponseBodiesEndpoint::Request>,
        RequestWithoutBodyJSONResponseBodiesEndpoint::Response>;

using ClientTestRequestWithoutBodyJSONResponseBodiesResponseWithHeaders =
    ClientTest<
        RequestWithoutBodyJSONResponseBodiesEndpoint,
        RequestWithoutBodyJSONResponseBodiesEndpoint::Request,
        WithHeaders<RequestWithoutBodyJSONResponseBodiesEndpoint::Response>>;

using ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesNoHeaders =
    ClientTest<
        ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint,
        ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Request,
        ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Response>;

using ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders =
    ClientTest<
        ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint,
        WithHeaders<
            ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Request>,
        ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Response>;

using ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders =
    ClientTest<
        ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint,
        ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Request,
        WithHeaders<ProtobufNonEmptyRequestBodyProtobufResponseBodiesEndpoint::
                        Response>>;

using ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesNoHeaders =
    ClientTest<
        ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint,
        ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint::Request,
        ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint::Response>;

using ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders =
    ClientTest<
        ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint,
        WithHeaders<
            ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint::Request>,
        ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint::Response>;

using ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders =
    ClientTest<
        ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint,
        ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint::Request,
        WithHeaders<
            ProtobufEmptyRequestBodyProtobufResponseBodiesEndpoint::Response>>;

using ClientTestRequestWithoutBodyProtobufResponseBodiesNoHeaders =
    ClientTest<RequestWithoutBodyProtobufResponseBodiesEndpoint,
               RequestWithoutBodyProtobufResponseBodiesEndpoint::Request,
               RequestWithoutBodyProtobufResponseBodiesEndpoint::Response>;

using ClientTestRequestWithoutBodyProtobufResponseBodiesRequestWithHeaders =
    ClientTest<
        RequestWithoutBodyProtobufResponseBodiesEndpoint,
        WithHeaders<RequestWithoutBodyProtobufResponseBodiesEndpoint::Request>,
        RequestWithoutBodyProtobufResponseBodiesEndpoint::Response>;

using ClientTestRequestWithoutBodyProtobufResponseBodiesResponseWithHeaders =
    ClientTest<RequestWithoutBodyProtobufResponseBodiesEndpoint,
               RequestWithoutBodyProtobufResponseBodiesEndpoint::Request,
               WithHeaders<
                   RequestWithoutBodyProtobufResponseBodiesEndpoint::Response>>;

using ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesNoHeaders =
    ClientTest<JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint,
               JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Request,
               JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Response>;

using ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders =
    ClientTest<
        JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint,
        WithHeaders<
            JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Request>,
        JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Response>;

using ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders =
    ClientTest<
        JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint,
        JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Request,
        WithHeaders<
            JSONNonEmptyRequestBodyProtobufResponseBodiesEndpoint::Response>>;

using ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesNoHeaders =
    ClientTest<ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint,
               ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request,
               ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>;

using ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders =
    ClientTest<
        ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint,
        WithHeaders<
            ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request>,
        ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>;

using ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders =
    ClientTest<
        ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint,
        ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request,
        WithHeaders<
            ProtobufNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>>;

using ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesNoHeaders =
    ClientTest<
        RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint,
        RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint::Request,
        RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint::Response>;

using ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesRequestWithHeaders =
    ClientTest<
        RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint,
        WithHeaders<
            RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint::Request>,
        RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint::Response>;

using ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesResponseWithHeaders =
    ClientTest<
        RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint,
        RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint::Request,
        WithHeaders<RequestWithoutBodyProtobufSuccessJSONErrorBodiesEndpoint::
                        Response>>;

using ClientTestRequestWithoutBodyResponseWithoutBodiesNoHeaders =
    ClientTest<RequestWithoutBodyResponseWithoutBodiesEndpoint,
               RequestWithoutBodyResponseWithoutBodiesEndpoint::Request,
               RequestWithoutBodyResponseWithoutBodiesEndpoint::Response>;

using ClientTestRequestWithoutBodyResponseWithoutBodiesRequestWithHeaders =
    ClientTest<
        RequestWithoutBodyResponseWithoutBodiesEndpoint,
        WithHeaders<RequestWithoutBodyResponseWithoutBodiesEndpoint::Request>,
        RequestWithoutBodyResponseWithoutBodiesEndpoint::Response>;

using ClientTestRequestWithoutBodyResponseWithoutBodiesResponseWithHeaders =
    ClientTest<
        RequestWithoutBodyResponseWithoutBodiesEndpoint,
        RequestWithoutBodyResponseWithoutBodiesEndpoint::Request,
        WithHeaders<RequestWithoutBodyResponseWithoutBodiesEndpoint::Response>>;

class ClientTestCancelableRequest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

using ClientTestCancelableRequestDeathTest = ClientTestCancelableRequest;

struct RetryOptionsTestCase {
  std::string test_name;
  internal::RetryOptions retry_options;
  // Total number of times the request is expected to be issued
  // (initial attempt + retries).
  std::size_t expected_num_requests;
};

class ClientTestRetryOptions
    : public testing::TestWithParam<RetryOptionsTestCase> {
 public:
  static constexpr auto kNameGenerator = [](const auto& info) {
    return info.param.test_name;
  };

 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

}  // namespace

TEST_P(ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesNoHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    JSONNonEmptyRequestBodyJSONResponseBodiesNoHeaders,
    ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesNoHeaders,
    ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesNoHeaders::
        kNameGenerator);

TEST_P(ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    JSONNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders,
    ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders,
    ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    JSONNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders,
    ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders,
    ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestJSONNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders::
        kNameGenerator);

TEST_P(ClientTestJSONEmptyRequestBodyJSONResponseBodiesNoHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    JSONEmptyRequestBodyJSONResponseBodiesNoHeaders,
    ClientTestJSONEmptyRequestBodyJSONResponseBodiesNoHeaders,
    ClientTestJSONEmptyRequestBodyJSONResponseBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestJSONEmptyRequestBodyJSONResponseBodiesNoHeaders::kNameGenerator);

TEST_P(ClientTestJSONEmptyRequestBodyJSONResponseBodiesRequestWithHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    JSONEmptyRequestBodyJSONResponseBodiesRequestWithHeaders,
    ClientTestJSONEmptyRequestBodyJSONResponseBodiesRequestWithHeaders,
    ClientTestJSONEmptyRequestBodyJSONResponseBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestJSONEmptyRequestBodyJSONResponseBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(ClientTestJSONEmptyRequestBodyJSONResponseBodiesResponseWithHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    JSONEmptyRequestBodyJSONResponseBodiesResponseWithHeaders,
    ClientTestJSONEmptyRequestBodyJSONResponseBodiesResponseWithHeaders,
    ClientTestJSONEmptyRequestBodyJSONResponseBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestJSONEmptyRequestBodyJSONResponseBodiesResponseWithHeaders::
        kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyJSONResponseBodiesNoHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyJSONResponseBodiesNoHeaders,
    ClientTestRequestWithoutBodyJSONResponseBodiesNoHeaders,
    ClientTestRequestWithoutBodyJSONResponseBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyJSONResponseBodiesNoHeaders::kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyJSONResponseBodiesRequestWithHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyJSONResponseBodiesRequestWithHeaders,
    ClientTestRequestWithoutBodyJSONResponseBodiesRequestWithHeaders,
    ClientTestRequestWithoutBodyJSONResponseBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyJSONResponseBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyJSONResponseBodiesResponseWithHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyJSONResponseBodiesResponseWithHeaders,
    ClientTestRequestWithoutBodyJSONResponseBodiesResponseWithHeaders,
    ClientTestRequestWithoutBodyJSONResponseBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyJSONResponseBodiesResponseWithHeaders::
        kNameGenerator);

TEST_P(ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesNoHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufNonEmptyRequestBodyProtobufResponseBodiesNoHeaders,
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesNoHeaders,
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesNoHeaders::
        kNameGenerator);

TEST_P(
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders,
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders,
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders,
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders,
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestProtobufNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders::
        kNameGenerator);

TEST_P(ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesNoHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufEmptyRequestBodyProtobufResponseBodiesNoHeaders,
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesNoHeaders,
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesNoHeaders::
        kNameGenerator);

TEST_P(
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders,
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders,
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders,
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders,
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestProtobufEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders::
        kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyProtobufResponseBodiesNoHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyProtobufResponseBodiesNoHeaders,
    ClientTestRequestWithoutBodyProtobufResponseBodiesNoHeaders,
    ClientTestRequestWithoutBodyProtobufResponseBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyProtobufResponseBodiesNoHeaders::
        kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyProtobufResponseBodiesRequestWithHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyProtobufResponseBodiesRequestWithHeaders,
    ClientTestRequestWithoutBodyProtobufResponseBodiesRequestWithHeaders,
    ClientTestRequestWithoutBodyProtobufResponseBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyProtobufResponseBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyProtobufResponseBodiesResponseWithHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyProtobufResponseBodiesResponseWithHeaders,
    ClientTestRequestWithoutBodyProtobufResponseBodiesResponseWithHeaders,
    ClientTestRequestWithoutBodyProtobufResponseBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyProtobufResponseBodiesResponseWithHeaders::
        kNameGenerator);

TEST_P(ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesNoHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    JSONNonEmptyRequestBodyProtobufResponseBodiesNoHeaders,
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesNoHeaders,
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesNoHeaders::
        kNameGenerator);

TEST_P(
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    JSONNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders,
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders,
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    JSONNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders,
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders,
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestJSONNonEmptyRequestBodyProtobufResponseBodiesResponseWithHeaders::
        kNameGenerator);

TEST_P(ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesNoHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufNonEmptyRequestBodyJSONResponseBodiesNoHeaders,
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesNoHeaders,
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesNoHeaders::
        kNameGenerator);

TEST_P(
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders,
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders,
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    ProtobufNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders,
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders,
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestProtobufNonEmptyRequestBodyJSONResponseBodiesResponseWithHeaders::
        kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesNoHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyProtobufSuccessJSONErrorBodiesNoHeaders,
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesNoHeaders,
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesNoHeaders::
        kNameGenerator);

TEST_P(
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesRequestWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyProtobufSuccessJSONErrorBodiesRequestWithHeaders,
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesRequestWithHeaders,
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesResponseWithHeaders,
    Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyProtobufSuccessJSONErrorBodiesResponseWithHeaders,
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesResponseWithHeaders,
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyProtobufSuccessJSONErrorBodiesResponseWithHeaders::
        kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyResponseWithoutBodiesNoHeaders, Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyResponseWithoutBodiesNoHeaders,
    ClientTestRequestWithoutBodyResponseWithoutBodiesNoHeaders,
    ClientTestRequestWithoutBodyResponseWithoutBodiesNoHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyResponseWithoutBodiesNoHeaders::kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyResponseWithoutBodiesRequestWithHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyResponseWithoutBodiesRequestWithHeaders,
    ClientTestRequestWithoutBodyResponseWithoutBodiesRequestWithHeaders,
    ClientTestRequestWithoutBodyResponseWithoutBodiesRequestWithHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyResponseWithoutBodiesRequestWithHeaders::
        kNameGenerator);

TEST_P(ClientTestRequestWithoutBodyResponseWithoutBodiesResponseWithHeaders,
       Send) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    RequestWithoutBodyResponseWithoutBodiesResponseWithHeaders,
    ClientTestRequestWithoutBodyResponseWithoutBodiesResponseWithHeaders,
    ClientTestRequestWithoutBodyResponseWithoutBodiesResponseWithHeaders::
        GenerateTestCases(),
    ClientTestRequestWithoutBodyResponseWithoutBodiesResponseWithHeaders::
        kNameGenerator);

TEST_F(ClientTestCancelableRequest, ResetPreventsCallback) {
  MockResponseFor<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>(
      test_url_loader_factory_,
      {.net_error = net::OK, .status_code = net::HTTP_OK});

  base::test::TestFuture<
      JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>
      cancelled_future;
  RequestHandle request_handle =
      Client<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>::Send<
          RequestCancelability::kCancelable>(
          test_url_loader_factory_,
          JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request(),
          cancelled_future.GetCallback());

  // Sent after the cancelled request, so once this one completes, the cancelled
  // one would have too.
  base::test::TestFuture<
      JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>
      future;
  Client<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>::Send(
      test_url_loader_factory_,
      JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request(),
      future.GetCallback());

  request_handle.reset();

  ASSERT_TRUE(future.Wait());
  EXPECT_FALSE(cancelled_future.IsReady());
}

TEST_F(ClientTestCancelableRequest,
       ResetInsideCallbackIsSafeForResponsesWithBodies) {
  MockResponseFor<RequestWithoutBodyJSONResponseBodiesEndpoint>(
      test_url_loader_factory_,
      {.net_error = net::OK, .status_code = net::HTTP_OK});

  base::test::TestFuture<void> future;
  RequestHandle request_handle =
      Client<RequestWithoutBodyJSONResponseBodiesEndpoint>::Send<
          RequestCancelability::kCancelable>(
          test_url_loader_factory_,
          RequestWithoutBodyJSONResponseBodiesEndpoint::Request(),
          base::BindLambdaForTesting(
              [&request_handle, done = future.GetCallback()](
                  RequestWithoutBodyJSONResponseBodiesEndpoint::
                      Response) mutable {
                request_handle.reset();
                std::move(done).Run();
              }));

  ASSERT_TRUE(future.Wait());
}

TEST_F(ClientTestCancelableRequest,
       ResetInsideCallbackIsSafeForResponsesWithoutBodies) {
  MockResponseFor<RequestWithoutBodyResponseWithoutBodiesEndpoint>(
      test_url_loader_factory_,
      {.net_error = net::OK, .status_code = net::HTTP_OK});

  base::test::TestFuture<void> future;
  RequestHandle request_handle =
      Client<RequestWithoutBodyResponseWithoutBodiesEndpoint>::Send<
          RequestCancelability::kCancelable>(
          test_url_loader_factory_,
          RequestWithoutBodyResponseWithoutBodiesEndpoint::Request(),
          base::BindLambdaForTesting(
              [&request_handle, done = future.GetCallback()](
                  RequestWithoutBodyResponseWithoutBodiesEndpoint::
                      Response) mutable {
                request_handle.reset();
                std::move(done).Run();
              }));

  ASSERT_TRUE(future.Wait());
}

TEST_F(ClientTestCancelableRequest, ResetAfterCompletionIsNoOp) {
  MockResponseFor<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>(
      test_url_loader_factory_,
      {.net_error = net::OK, .status_code = net::HTTP_OK});

  base::test::TestFuture<
      JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>
      future;
  RequestHandle request_handle =
      Client<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>::Send<
          RequestCancelability::kCancelable>(
          test_url_loader_factory_,
          JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request(),
          future.GetCallback());

  ASSERT_TRUE(future.Wait());
  request_handle.reset();
}

TEST_F(ClientTestCancelableRequestDeathTest, ResetOnAnotherSequenceCrashes) {
  base::test::TestFuture<
      JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>
      future;
  RequestHandle request_handle =
      Client<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>::Send<
          RequestCancelability::kCancelable>(
          test_url_loader_factory_,
          JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request(),
          future.GetCallback());

  EXPECT_CHECK_DEATH({
    base::Thread thread("other-sequence");
    CHECK(thread.Start());
    thread.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce([](RequestHandle) {}, std::move(request_handle)));
    thread.FlushForTesting();
  });
}

TEST(ClientTestUrlReplacements, OverridesReachTheOutgoingRequest) {
  base::test::TaskEnvironment task_environment;
  network::TestURLLoaderFactory test_url_loader_factory;

  // JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::URL() is
  // https://example.com/api/query; the request's url_replacements override the
  // host, path, and query.
  UrlReplacements url_replacements;
  url_replacements.SetHost("host.example.com")
      .SetPath("/path/to/resource")
      .SetQuery("key=value");

  // Serve a response at the overridden URL so the request can complete.
  MockResponseFor<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>(
      test_url_loader_factory,
      {.net_error = net::OK, .status_code = net::HTTP_OK}, url_replacements);

  // Assert against a hard-coded literal (not url_replacements.Apply()) so a bug
  // in Apply() can't hide by shifting both the mock and the expectation.
  test_url_loader_factory.SetInterceptor(base::BindLambdaForTesting(
      [&](const network::ResourceRequest& resource_request) {
        EXPECT_EQ(resource_request.url,
                  GURL("https://host.example.com/path/to/resource?key=value"));
      }));

  JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request request;
  request.url_replacements = std::move(url_replacements);

  base::test::TestFuture<
      JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>
      future;
  Client<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>::Send(
      test_url_loader_factory, std::move(request), future.GetCallback());
  // Wait() only resolves once the request completed, which requires the
  // interceptor to have run - so the EXPECT_EQ above is guaranteed to have been
  // reached.
  ASSERT_TRUE(future.Wait());
}

TEST(ClientTestMaxBodySize, ExceedingItFailsWithInsufficientResources) {
  base::test::TaskEnvironment task_environment;
  network::TestURLLoaderFactory test_url_loader_factory;

  MockResponseFor<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>(
      test_url_loader_factory,
      {.net_error = net::OK, .status_code = net::HTTP_OK, .body = [] {
         JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response::
             SuccessBody body;
         body.success = "success";
         return body;
       }()});

  JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request request;
  request.max_body_size = 1;

  base::test::TestFuture<
      JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>
      future;
  Client<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>::Send(
      test_url_loader_factory, std::move(request), future.GetCallback());

  JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response expected_response;
  expected_response.net_error = net::ERR_INSUFFICIENT_RESOURCES;
  EXPECT_THAT(future.Take(), ResponseEq(expected_response));
}

TEST(ClientTestMaxBodySize, DoesNotApplyToResponsesWithoutBodies) {
  base::test::TaskEnvironment task_environment;
  network::TestURLLoaderFactory test_url_loader_factory;

  test_url_loader_factory.AddResponse(
      RequestWithoutBodyResponseWithoutBodiesEndpoint::URL().spec(),
      "longer than max_body_size", net::HTTP_OK);

  RequestWithoutBodyResponseWithoutBodiesEndpoint::Request request;
  request.max_body_size = 1;

  base::test::TestFuture<
      RequestWithoutBodyResponseWithoutBodiesEndpoint::Response>
      future;
  Client<RequestWithoutBodyResponseWithoutBodiesEndpoint>::Send(
      test_url_loader_factory, std::move(request), future.GetCallback());

  EXPECT_THAT(
      future.Take(),
      ResponseEq(RequestWithoutBodyResponseWithoutBodiesEndpoint::Response{
          .net_error = net::OK, .status_code = net::HTTP_OK}));
}

TEST(ClientTestResponseHeaders, NotSetWithoutStatusCode) {
  base::test::TaskEnvironment task_environment;
  network::TestURLLoaderFactory test_url_loader_factory;

  MockResponseFor<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>(
      test_url_loader_factory,
      {.net_error = net::OK, .status_code = net::HTTP_OK, .body = [] {
         JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response::
             SuccessBody body;
         body.success = "success";
         return body;
       }()});

  JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request request;
  // Fails the request after the response headers have been received.
  request.max_body_size = 1;

  base::test::TestFuture<
      WithHeaders<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>>
      future;
  Client<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>::Send(
      test_url_loader_factory, std::move(request), future.GetCallback());

  WithHeaders<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>
      expected_response;
  expected_response.net_error = net::ERR_INSUFFICIENT_RESOURCES;
  EXPECT_THAT(future.Take(), ResponseEq(expected_response));
}

TEST_P(ClientTestRetryOptions, Retry) {
  const auto& test_case = GetParam();

  // Always respond with 5xx so every attempt is retried until max_retries is
  // exhausted, then count how many times the request was actually issued.
  MockResponseFor<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>(
      test_url_loader_factory_,
      {.net_error = net::OK, .status_code = net::HTTP_INTERNAL_SERVER_ERROR});

  std::size_t num_requests = 0;
  test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&](const network::ResourceRequest&) { ++num_requests; }));

  JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Request request;
  request.retry_options = test_case.retry_options;

  base::test::TestFuture<
      JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint::Response>
      future;
  Client<JSONNonEmptyRequestBodyJSONResponseBodiesEndpoint>::Send(
      test_url_loader_factory_, std::move(request), future.GetCallback());
  ASSERT_TRUE(future.Wait());

  EXPECT_EQ(num_requests, test_case.expected_num_requests);
}

INSTANTIATE_TEST_SUITE_P(
    RetryOptions,
    ClientTestRetryOptions,
    testing::Values(
        // The default retry_options must not retry.
        RetryOptionsTestCase{.test_name = "does_not_retry_by_default",
                             .retry_options = {},
                             .expected_num_requests = 1},
        RetryOptionsTestCase{
            .test_name = "retries_on_server_error_up_to_max_retries",
            .retry_options = {.max_retries = 2,
                              .retry_mode =
                                  network::SimpleURLLoader::RETRY_ON_5XX},
            // Initial request + 2 retries.
            .expected_num_requests = 3}),
    ClientTestRetryOptions::kNameGenerator);

}  // namespace endpoint_client
