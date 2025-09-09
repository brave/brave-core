/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/client.h"

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/to_string.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/types/always_false.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_request.mojom-shared.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using endpoints::Endpoint;
using endpoints::For;
using endpoints::PATCH;
using endpoints::POST;

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

inline constexpr char kRequest1Key[] = "request1";
inline constexpr char kRequest2Key[] = "request2";

inline constexpr char kResponse1Key[] = "response1";
inline constexpr char kResponse2Key[] = "response2";
inline constexpr char kResponse3Key[] = "response3";

inline constexpr char kError1Key[] = "error1";
inline constexpr char kError2Key[] = "error2";
}  // namespace

namespace endpoint_client {

using Request1 = Message<kRequest1Key>;
using Request2 = Message<kRequest2Key>;

using Response1 = Message<kResponse1Key>;
using Response2 = Message<kResponse2Key>;
using Response3 = Message<kResponse3Key>;

using Error1 = Message<kError1Key>;
using Error2 = Message<kError2Key>;

struct TestEndpoint
    : Endpoint<
          // POST<Request1> =>
          //   expected<
          //     optional<variant<Response1, Response2>>,
          //     optional<Error1>
          //   >
          For<POST<Request1>>::RespondsWith<Response1,
                                            Response2>::ErrorsWith<Error1>,
          // PATCH<Request2> =>
          //   expected<
          //     optional<Response3>,
          //     optional<variant<Error1, Error2>>
          //   >
          For<PATCH<Request2>>::RespondsWith<Response3>::ErrorsWith<Error1,
                                                                    Error2>,
          // PATCH<Request1> =>
          //   expected<
          //     optional<Response2>,
          //     optional<Error2>
          //   >
          For<PATCH<Request1>>::RespondsWith<Response2>::ErrorsWith<Error2>> {
  static GURL URL() { return GURL("https://example.com/api/query"); }
};

struct TestCase {
  std::variant<POST<Request1>, PATCH<Request2>, PATCH<Request1>> request;
  net::HttpStatusCode status_code;
  std::string raw_reply;
  std::variant<TestEndpoint::ExpectedFor<POST<Request1>>,
               TestEndpoint::ExpectedFor<PATCH<Request2>>,
               TestEndpoint::ExpectedFor<PATCH<Request1>>>
      parsed_reply;
};

class EndpointClientTest : public testing::TestWithParam<TestCase> {
 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

TEST_P(EndpointClientTest, Send) {
  const TestCase& test_case = GetParam();

  test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&](const network::ResourceRequest& resource_request) {
        // Always add a response immediately so the request does not hang,
        // even if validations below fail early with ADD_FAILURE().
        test_url_loader_factory_.AddResponse(resource_request.url.spec(),
                                             test_case.raw_reply,
                                             test_case.status_code);

        // Method
        EXPECT_EQ(resource_request.method, "POST");
        // URL
        EXPECT_EQ(resource_request.url, GURL("https://example.com/api/query"));
        // Request body
        if (!resource_request.request_body) {
          return ADD_FAILURE() << "resource_request.request_body is nullptr !";
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
        std::visit(
            [&](const auto& test_case_request) {
              EXPECT_EQ(*request, test_case_request.text);
            },
            test_case.request);
        // Headers
        EXPECT_EQ(resource_request.headers.GetHeader("Content-Type"),
                  "application/json");
      }));

  base::RunLoop run_loop;

  std::visit(
      [&]<typename Request>(Request request) {
        using Expected = TestEndpoint::ExpectedFor<Request>;
        using Callback = TestEndpoint::CallbackFor<Request>;

        ASSERT_TRUE(std::holds_alternative<Expected>(test_case.parsed_reply));

        base::MockCallback<Callback> callback;
        EXPECT_CALL(callback, Run(test_case.status_code,
                                  std::get<Expected>(test_case.parsed_reply)))
            .Times(1)
            .WillOnce([&] { run_loop.Quit(); });
        Client<TestEndpoint>::Send(
            test_url_loader_factory_.GetSafeWeakWrapper(), std::move(request),
            callback.Get());
      },
      test_case.request);

  run_loop.Run();
}

INSTANTIATE_TEST_SUITE_P(
    EndpointClientTestCases,
    EndpointClientTest,
    testing::Values(TestCase{.request =
                                 [] {
                                   POST<Request1> request;
                                   request.text = "Request1";
                                   return request;
                                 }(),
                             .status_code = net::HTTP_OK,
                             .raw_reply = R"({"response": "some response"})"},
                    TestCase{.request =
                                 [] {
                                   PATCH<Request2> request;
                                   request.text = "Request2";
                                   return request;
                                 }(),
                             .status_code = net::HTTP_CREATED,
                             .raw_reply = R"({"invalid": response})"},
                    TestCase{.request =
                                 [] {
                                   PATCH<Request1> request;
                                   request.text = "Request1";
                                   return request;
                                 }(),
                             .status_code = net::HTTP_BAD_REQUEST,
                             .raw_reply = R"({"error": "some error"})"}),
    [](const auto& info) {
      std::string name;
      base::ReplaceChars("HTTP_" + base::ToString(info.param.status_code) +
                             "_" +
                             net::GetHttpReasonPhrase(info.param.status_code),
                         " ", "_", &name);
      return name;
    });

}  // namespace endpoint_client
