/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/brave_vpn_api_client.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/extend.h"
#include "base/functional/callback.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/test_support.h"
#include "brave/components/brave_vpn/browser/v2/api/error_body.h"
#include "brave/components/brave_vpn/browser/v2/api/purchase_endpoints.h"
#include "brave/components/brave_vpn/browser/v2/api/raw_json_response_body.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {
namespace {
using brave_account::endpoint_client::MockResponseFor;

constexpr char kTestProductType[] = "test-product-type";
constexpr char kTestProductId[] = "test-product-id";
constexpr char kTestValidationMethod[] = "test-validation-method";
constexpr char kTestPurchaseToken[] = "test-purchase-token";
constexpr char kTestBundleId[] = "test-bundle-id";
constexpr char kTestSkusCredential[] = "test-skus-credential";
constexpr char kTestSubscriberCredential[] = "test-subscriber-credential";
constexpr char kTestEnvironment[] = "test-env";

// Canonical JSON (compact, keys sorted): MockResponseFor re-serializes the
// body via ToValue() and the client re-parses it via FromValue(), so only
// already-canonical literals round-trip back to an identical string.
constexpr char kTestSuccessJson[] = R"({"receipt":"valid"})";
constexpr char kTestErrorJson[] = R"({"error":"invalid"})";
}  // namespace

template <typename TestCase>
class BraveVpnApiClientTest : public testing::TestWithParam<TestCase> {
 public:
  // Fires a client API method and blocks until its callback runs, returning the
  // result the callback was invoked with. Works for any method shaped like
  //   void Method(base::OnceCallback<void(SomeResultType)> callback, args...)
  template <typename Class,
            typename ResultType,
            typename... MethodArgs,
            typename... Args>
  auto CallClientApi(void (Class::*method)(base::OnceCallback<void(ResultType)>,
                                           MethodArgs...),
                     Args&&... args) {
    base::test::TestFuture<ResultType> future;
    (client_.*method)(future.GetCallback(), std::forward<Args>(args)...);
    return future.Take();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  BraveVpnApiClient client_{url_loader_factory_.GetSafeWeakWrapper()};
};

// The two "unrecoverable response" cases are identical for every endpoint.
template <typename TestCase>
std::vector<TestCase> WithCommonUnrecoverableCases(
    std::vector<TestCase> endpoint_specific_cases) {
  std::vector<TestCase> cases = {
      // A transport failure surfaces the net-error description, not an HTTP
      // status.
      TestCase{
          .test_name = "TransportErrorReturnsNetErrorString",
          .response = {.net_error = net::ERR_TIMED_OUT},
          .expected = base::unexpected(net::ErrorToString(net::ERR_TIMED_OUT))},
      // A non-2xx response whose body is not a parseable error falls back to
      // the HTTP status description (the status_code branch of
      // MaybeDescribeUnrecoverableResponse).
      TestCase{.test_name = "HttpErrorWithUnparseableBodyDescribesStatus",
               .response = {.net_error = net::OK,
                            .status_code = net::HTTP_INTERNAL_SERVER_ERROR},
               .expected = base::unexpected(absl::StrFormat(
                   "HTTP %d %s: body missing or failed to parse",
                   net::HTTP_INTERNAL_SERVER_ERROR,
                   net::GetHttpReasonPhrase(net::HTTP_INTERNAL_SERVER_ERROR)))},
  };
  base::Extend(cases, std::move(endpoint_specific_cases));
  return cases;
}

struct GetSubscriberCredentialTestCase {
  std::string test_name;
  endpoints::GetSubscriberCredential::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientGetSubscriberCredentialTest =
    BraveVpnApiClientTest<GetSubscriberCredentialTestCase>;

TEST_P(BraveVpnApiClientGetSubscriberCredentialTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  MockResponseFor<endpoints::GetSubscriberCredential>(url_loader_factory_,
                                                      test_case.response);
  EXPECT_EQ(
      CallClientApi(&BraveVpnApiClient::GetSubscriberCredential,
                    kTestProductType, kTestProductId, kTestValidationMethod,
                    kTestPurchaseToken, kTestBundleId),
      test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientGetSubscriberCredentialTest,
    testing::ValuesIn(WithCommonUnrecoverableCases<
                      GetSubscriberCredentialTestCase>(
        {// A 2xx response with a parseable success body yields the subscriber
         // credential.
         GetSubscriberCredentialTestCase{
             .test_name = "Success",
             .response = {.net_error = net::OK,
                          .status_code = net::HTTP_OK,
                          .body =
                              endpoints::GetSubscriberCredentialSuccessBody{
                                  .subscriber_credential =
                                      kTestSubscriberCredential}},
             .expected = base::ok(kTestSubscriberCredential)},
         // A non-2xx response with a parseable error body surfaces its title.
         GetSubscriberCredentialTestCase{
             .test_name = "RequestError",
             .response = {.net_error = net::OK,
                          .status_code = net::HTTP_BAD_REQUEST,
                          .body = base::unexpected(endpoints::VpnErrorBody{
                              .error_title = "Token no longer valid.",
                              .error_message = "gone"})},
             .expected = base::unexpected("Token no longer valid.")}})),
    [](const auto& info) { return info.param.test_name; });

struct GetSubscriberCredentialV12TestCase {
  std::string test_name;
  endpoints::GetSubscriberCredentialV12::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientGetSubscriberCredentialV12Test =
    BraveVpnApiClientTest<GetSubscriberCredentialV12TestCase>;

TEST_P(BraveVpnApiClientGetSubscriberCredentialV12Test, MapsResponseToResult) {
  const auto& test_case = GetParam();
  MockResponseFor<endpoints::GetSubscriberCredentialV12>(url_loader_factory_,
                                                         test_case.response);
  EXPECT_EQ(CallClientApi(&BraveVpnApiClient::GetSubscriberCredentialV12,
                          kTestSkusCredential, kTestEnvironment),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientGetSubscriberCredentialV12Test,
    testing::ValuesIn(WithCommonUnrecoverableCases<
                      GetSubscriberCredentialV12TestCase>(
        {// A 2xx response with a parseable success body yields the subscriber
         // credential.
         GetSubscriberCredentialV12TestCase{
             .test_name = "Success",
             .response = {.net_error = net::OK,
                          .status_code = net::HTTP_OK,
                          .body =
                              endpoints::GetSubscriberCredentialSuccessBody{
                                  .subscriber_credential =
                                      kTestSubscriberCredential}},
             .expected = base::ok(kTestSubscriberCredential)},
         // A non-2xx response with a parseable error body surfaces its title.
         GetSubscriberCredentialV12TestCase{
             .test_name = "RequestError",
             .response = {.net_error = net::OK,
                          .status_code = net::HTTP_BAD_REQUEST,
                          .body = base::unexpected(endpoints::VpnErrorBody{
                              .error_title = "Token no longer valid.",
                              .error_message = "gone"})},
             .expected = base::unexpected("Token no longer valid.")}})),
    [](const auto& info) { return info.param.test_name; });

struct VerifyPurchaseTokenTestCase {
  std::string test_name;
  endpoints::VerifyPurchaseToken::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientVerifyPurchaseTokenTest =
    BraveVpnApiClientTest<VerifyPurchaseTokenTestCase>;

TEST_P(BraveVpnApiClientVerifyPurchaseTokenTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  MockResponseFor<endpoints::VerifyPurchaseToken>(url_loader_factory_,
                                                  test_case.response);
  EXPECT_EQ(
      CallClientApi(&BraveVpnApiClient::VerifyPurchaseToken, kTestPurchaseToken,
                    kTestProductId, kTestProductType, kTestBundleId),
      test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientVerifyPurchaseTokenTest,
    testing::ValuesIn(WithCommonUnrecoverableCases<VerifyPurchaseTokenTestCase>(
        {// 2xx body is forwarded verbatim.
         VerifyPurchaseTokenTestCase{
             .test_name = "Success",
             .response = {.net_error = net::OK,
                          .status_code = net::HTTP_OK,
                          .body = base::ok(endpoints::RawJsonResponseBody{
                              .json = kTestSuccessJson})},
             .expected = base::ok(kTestSuccessJson)},
         // non-2xx error body is *also* forwarded verbatim (raw on both sides),
         // not collapsed to a VpnErrorBody title.
         VerifyPurchaseTokenTestCase{
             .test_name = "RequestError",
             .response = {.net_error = net::OK,
                          .status_code = net::HTTP_BAD_REQUEST,
                          .body =
                              base::unexpected(endpoints::RawJsonResponseBody{
                                  .json = kTestErrorJson})},
             .expected = base::unexpected(kTestErrorJson)}})),
    [](const auto& info) { return info.param.test_name; });

}  // namespace brave_vpn::v2
