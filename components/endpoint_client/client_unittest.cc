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
#include "base/test/test_future.h"
#include "base/types/always_false.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/endpoint_client/methods.h"
#include "brave/components/endpoint_client/with_headers.h"
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

  const char* GetKey() const { return Key; }

  std::string text;
};

constexpr char kRequest1Key[] = "request1";
constexpr char kRequest2Key[] = "request2";

constexpr char kResponse1Key[] = "response1";
constexpr char kResponse2Key[] = "response2";

constexpr char kError1Key[] = "error1";
constexpr char kError2Key[] = "error2";
}  // namespace

namespace endpoint_client {

using Request1 = Message<kRequest1Key>;
using Request2 = Message<kRequest2Key>;

using Response1 = Message<kResponse1Key>;
using Response2 = Message<kResponse2Key>;

using Error1 = Message<kError1Key>;
using Error2 = Message<kError2Key>;

using TestEndpoint = Endpoint<
    "example",
    "/api/query",
    // POST<Request1> =>
    //   expected<
    //     optional<variant<Response1, Response2>>,
    //     optional<Error1>
    //   >
    For<POST<Request1>>::ReturnsWith<Response1, Response2>::FailsWith<Error1>,
    // PATCH<Request2> =>
    //   expected<
    //     optional<Response1>,
    //     optional<variant<Error1, Error2>>
    //   >
    For<PATCH<Request2>>::ReturnsWith<Response1>::FailsWith<Error1, Error2>,
    // PATCH<Request1> =>
    //   expected<
    //     optional<Response1>,
    //     optional<Error1>
    //   >
    For<PATCH<Request1>>::ReturnsWith<Response1>::FailsWith<Error1>>;

struct TestCase {
  std::variant<POST<Request1>, PATCH<Request2>, WithHeaders<PATCH<Request1>>>
      request;
  net::HttpStatusCode status_code;
  std::string raw_reply;
  std::variant<TestEndpoint::EntryFor<POST<Request1>>::Expected,
               TestEndpoint::EntryFor<PATCH<Request2>>::Expected,
               TestEndpoint::EntryFor<PATCH<Request1>>::Expected>
      parsed_reply;
};

class EndpointClientTest : public testing::TestWithParam<TestCase> {
 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

TEST_P(EndpointClientTest, Send) {
  const TestCase& test_case = GetParam();

  std::visit(
      [&]<typename Request>(const Request& request) {
        test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
            [&](const network::ResourceRequest& resource_request) {
              // Always add a response immediately so the request does not hang,
              // even if validations below fail early with ADD_FAILURE().
              test_url_loader_factory_.AddResponse(resource_request.url.spec(),
                                                   test_case.raw_reply,
                                                   test_case.status_code);
              // Method
              EXPECT_EQ(resource_request.method, request.Method());
              // URL
              EXPECT_EQ(resource_request.url,
                        GURL("https://example.brave.com/api/query"));
              // Request body
              if (!resource_request.request_body) {
                return ADD_FAILURE()
                       << "resource_request.request_body is nullptr !";
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
              const auto* key = dict.FindString(request.GetKey());
              EXPECT_NE(key, nullptr);
              EXPECT_EQ(*key, request.text);
              // Headers
              EXPECT_EQ(resource_request.headers.GetHeader("Content-Type"),
                        "application/json");
            }));

        using Entry = TestEndpoint::EntryFor<Request>;
        using Expected = Entry::Expected;
        using Response = Entry::Response;

        base::test::TestFuture<Expected> future;
        Client<TestEndpoint>::Send(
            test_url_loader_factory_.GetSafeWeakWrapper(), request,
            future.GetCallback());
        auto expected = future.Take();
        ASSERT_TRUE(std::holds_alternative<Expected>(test_case.parsed_reply));
        const Expected& got = std::get<Expected>(test_case.parsed_reply);
        EXPECT_EQ(expected, got);
        // TODO(sszaloki): currently, if a Response/Error type is
        // WithHeaders<T>, EXPECT_EQ() will use T::operator==(), which
        // WithHeaders<T> inherits from T. Check headers for equality.
        // Below is a workaround, as HttpResponseHeaders::StrictlyEquals()
        // doesn't work because of a space difference.
        if constexpr (base::is_instantiation<Response, WithHeaders>) {
          if (expected.has_value() && expected.value() &&
              expected.value()->headers) {
            ASSERT_TRUE(got.has_value() && got.value() && got.value()->headers);

            const auto& expected_headers = expected.value()->headers;
            const auto& got_headers = got.value()->headers;

            std::string expected_raw_no_ws;
            std::string got_raw_no_ws;
            base::RemoveChars(expected_headers->raw_headers(), " ",
                              &expected_raw_no_ws);
            base::RemoveChars(got_headers->raw_headers(), " ", &got_raw_no_ws);

            EXPECT_EQ(expected_raw_no_ws, got_raw_no_ws);
          }
        }

        // Client<TestEndpoint>::Send(
        //     test_url_loader_factory_.GetSafeWeakWrapper(),
        //     PATCH<Request2>{{"POST<Request2>"}},
        //     base::BindOnce(
        //         [](base::expected<
        //             std::optional<Response1>,
        //             std::optional<std::variant<Error1, Error2>>>) {}));
      },
      test_case.request);
}

INSTANTIATE_TEST_SUITE_P(
    EndpointClientTestCases,
    EndpointClientTest,
    testing::Values(
        // POST<Request1>
        TestCase{
            // ==> Response1
            .request = POST<Request1>{{"POST<Request1>"}},
            .status_code = net::HTTP_OK,
            .raw_reply = R"({"response1": "Response1"})",
            .parsed_reply = TestEndpoint::EntryFor<POST<Request1>>::Expected(
                Response1("Response1")),
        },
        TestCase{
            // ==> Response2
            .request = POST<Request1>{{"POST<Request1>"}},
            .status_code = net::HTTP_CREATED,
            .raw_reply = R"({"response2": "Response2"})",
            .parsed_reply = TestEndpoint::EntryFor<POST<Request1>>::Expected(
                Response2("Response2")),
        },
        TestCase{
            // ==> unsupported response
            .request = POST<Request1>{{"POST<Request1>"}},
            .status_code = net::HTTP_ACCEPTED,
            .raw_reply = R"({"response3": "Response3"})",
            .parsed_reply =
                TestEndpoint::EntryFor<POST<Request1>>::Expected(std::nullopt),
        },
        TestCase{
            // ==> Error1
            .request = POST<Request1>{{"POST<Request1>"}},
            .status_code = net::HTTP_MULTIPLE_CHOICES,
            .raw_reply = R"({"error1": "Error1"})",
            .parsed_reply = TestEndpoint::EntryFor<POST<Request1>>::Expected(
                base::unexpected(Error1("Error1"))),
        },
        TestCase{
            // ==> unsupported error
            .request = POST<Request1>{{"POST<Request1>"}},
            .status_code = net::HTTP_MOVED_PERMANENTLY,
            .raw_reply = R"({"error2": "Error2"})",
            .parsed_reply = TestEndpoint::EntryFor<POST<Request1>>::Expected(
                base::unexpected(std::nullopt)),
        },
        // PATCH<Request2>
        TestCase{
            // ==> Response1
            .request = PATCH<Request2>{{"PATCH<Request2>"}},
            .status_code = net::HTTP_NON_AUTHORITATIVE_INFORMATION,
            .raw_reply = R"({"response1": "Response1"})",
            .parsed_reply = TestEndpoint::EntryFor<PATCH<Request2>>::Expected(
                Response1("Response1"))},
        TestCase{
            // ==> Error1
            .request = PATCH<Request2>{{"PATCH<Request2>"}},
            .status_code = net::HTTP_FOUND,
            .raw_reply = R"({"error1": "Error1"})",
            .parsed_reply = TestEndpoint::EntryFor<PATCH<Request2>>::Expected(
                base::unexpected(Error1("Error1")))},
        TestCase{
            // ==> Error2
            .request = PATCH<Request2>{{"PATCH<Request2>"}},
            .status_code = net::HTTP_SEE_OTHER,
            .raw_reply = R"({"error2": "Error2"})",
            .parsed_reply = TestEndpoint::EntryFor<PATCH<Request2>>::Expected(
                base::unexpected(Error2("Error2")))},
        // WithHeaders<PATCH<Request1>>
        TestCase{
            // ==> WithHeaders<Response1>
            .request =
                WithHeaders<PATCH<Request1>>{{"WithHeaders<PATCH<Request1>>"}},
            .status_code = net::HTTP_NO_CONTENT,
            .raw_reply = R"({"response1": "Response1"})",
            .parsed_reply =
                [] {
                  WithHeaders<Response1> response;
                  response.text = "Response1";
                  response.headers =
                      net::HttpResponseHeaders::Builder(net::HttpVersion(1, 1),
                                                        "204 No Content")
                          .AddHeader("Content-type", "text/html")
                          .Build();
                  return TestEndpoint::EntryFor<WithHeaders<PATCH<Request1>>>::
                      Expected(std::move(response));
                }()},
        TestCase{
            // ==> Error1
            .request =
                WithHeaders<PATCH<Request1>>{{"WithHeaders<PATCH<Request1>>"}},
            .status_code = net::HTTP_NOT_MODIFIED,
            .raw_reply = R"({"error1": "Error1"})",
            .parsed_reply = TestEndpoint::EntryFor<PATCH<Request1>>::Expected(
                base::unexpected(Error1("Error1")))}),
    [](const auto& info) {
      return std::visit(
          [&](const auto& request) {
            std::string name;
            base::ReplaceChars(
                request.text + "_HTTP_" +
                    base::ToString(info.param.status_code) + "_" +
                    net::GetHttpReasonPhrase(info.param.status_code),
                "<>- ", "_", &name);
            return name;
          },
          info.param.request);
    });

}  // namespace endpoint_client
