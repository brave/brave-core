/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/brave_vpn_api_client.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/extend.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/test_support.h"
#include "brave/components/brave_account/endpoint_client/url_replacements.h"
#include "brave/components/brave_vpn/browser/v2/api/device_endpoints.h"
#include "brave/components/brave_vpn/browser/v2/api/error_body.h"
#include "brave/components/brave_vpn/browser/v2/api/purchase_endpoints.h"
#include "brave/components/brave_vpn/browser/v2/api/raw_json_response_body.h"
#include "brave/components/brave_vpn/browser/v2/api/region_endpoints.h"
#include "brave/components/brave_vpn/browser/v2/api/support_endpoints.h"
#include "brave/components/brave_vpn/browser/v2/api/transport_protocol.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {
namespace {
using brave_account::endpoint_client::MockResponseFor;

constexpr char kTestErrorTitle[] = "Test error title.";
constexpr char kTestErrorMessage[] = "test error message";
constexpr char kTestProductType[] = "test-product-type";
constexpr char kTestProductId[] = "test-product-id";
constexpr char kTestValidationMethod[] = "test-validation-method";
constexpr char kTestPurchaseToken[] = "test-purchase-token";
constexpr char kTestBundleId[] = "test-bundle-id";
constexpr char kTestSkusCredential[] = "test-skus-credential";
constexpr char kTestSubscriberCredential[] = "test-subscriber-credential";
constexpr char kTestEnvironment[] = "test-env";
constexpr char kTestEmail[] = "user@example.com";
constexpr char kTestSubject[] = "Help";
constexpr char kTestBody[] = "It doesn't connect.";
constexpr char kTestTimezone[] = "America/Los_Angeles";
constexpr char kTestRegion[] = "us-east";
constexpr char kTestRegionPrecision[] = "city-by-country";
constexpr char kTestHostname[] = "sgw-node.guardianapp.com";
constexpr char kTestClientId[] = "test-client-id";
constexpr char kTestApiAuthToken[] = "test-api-auth-token";
constexpr char kTestPublicKey[] = "test-public-key";
constexpr char kTestMultihopExitRegion[] = "us-west";

// Canonical JSON (compact, keys sorted): MockResponseFor re-serializes the
// body via ToValue() and the client re-parses it via FromValue(), so only
// already-canonical literals round-trip back to an identical string.
constexpr char kTestSuccessJson[] = R"({"receipt":"valid"})";
constexpr char kTestErrorJson[] = R"({"error":"invalid"})";
}  // namespace

class BraveVpnApiClientTestBase : public testing::Test {
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

template <typename TestCase>
class BraveVpnApiClientTest : public BraveVpnApiClientTestBase,
                              public testing::WithParamInterface<TestCase> {};

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

// Two "Success" and "RequestError" cases are identical for every endpoint whose
// response is Response<RawJsonResponseBody, VpnErrorBody>.
template <typename TestCase>
std::vector<TestCase> WithCommonRawJsonCases(
    std::vector<TestCase> endpoint_specific_cases) {
  std::vector<TestCase> cases = {
      // 2xx body is forwarded verbatim.
      TestCase{.test_name = "Success",
               .response = {.net_error = net::OK,
                            .status_code = net::HTTP_OK,
                            .body = base::ok(endpoints::RawJsonResponseBody{
                                .json = kTestSuccessJson})},
               .expected = base::ok(kTestSuccessJson)},
      // A non-2xx response with a parseable error body surfaces its title.
      TestCase{.test_name = "RequestError",
               .response = {.net_error = net::OK,
                            .status_code = net::HTTP_BAD_REQUEST,
                            .body = base::unexpected(endpoints::VpnErrorBody{
                                .error_title = kTestErrorTitle,
                                .error_message = kTestErrorMessage})},
               .expected = base::unexpected(kTestErrorTitle)},
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
         // A non-2xx error body is *also* forwarded verbatim (raw on both
         // sides), not collapsed to a VpnErrorBody title.
         VerifyPurchaseTokenTestCase{
             .test_name = "RequestError",
             .response = {.net_error = net::OK,
                          .status_code = net::HTTP_BAD_REQUEST,
                          .body =
                              base::unexpected(endpoints::RawJsonResponseBody{
                                  .json = kTestErrorJson})},
             .expected = base::unexpected(kTestErrorJson)}})),
    [](const auto& info) { return info.param.test_name; });

struct CreateSupportTicketTestCase {
  std::string test_name;
  endpoints::CreateSupportTicket::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientCreateSupportTicketTest =
    BraveVpnApiClientTest<CreateSupportTicketTestCase>;

TEST_P(BraveVpnApiClientCreateSupportTicketTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  MockResponseFor<endpoints::CreateSupportTicket>(url_loader_factory_,
                                                  test_case.response);
  EXPECT_EQ(CallClientApi(&BraveVpnApiClient::CreateSupportTicket, kTestEmail,
                          kTestSubject, kTestBody, kTestSubscriberCredential,
                          kTestTimezone),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientCreateSupportTicketTest,
    testing::ValuesIn(WithCommonUnrecoverableCases<CreateSupportTicketTestCase>(
        WithCommonRawJsonCases<CreateSupportTicketTestCase>({}))),
    [](const auto& info) { return info.param.test_name; });

struct GetServerRegionsTestCase {
  std::string test_name;
  endpoints::GetServerRegions::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientGetServerRegionsTest =
    BraveVpnApiClientTest<GetServerRegionsTestCase>;

TEST_P(BraveVpnApiClientGetServerRegionsTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  brave_account::endpoint_client::UrlReplacements url_replacements;
  url_replacements.SetPath(base::StrCat(
      {endpoints::GetServerRegions::URL().path(), "/", kTestRegionPrecision}));
  MockResponseFor<endpoints::GetServerRegions>(
      url_loader_factory_, test_case.response, url_replacements);
  EXPECT_EQ(
      CallClientApi(&BraveVpnApiClient::GetServerRegions, kTestRegionPrecision),
      test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientGetServerRegionsTest,
    testing::ValuesIn(WithCommonUnrecoverableCases<GetServerRegionsTestCase>(
        WithCommonRawJsonCases<GetServerRegionsTestCase>({}))),
    [](const auto& info) { return info.param.test_name; });

struct GetTimezonesForRegionsTestCase {
  std::string test_name;
  endpoints::GetTimezonesForRegions::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientGetTimezonesForRegionsTest =
    BraveVpnApiClientTest<GetTimezonesForRegionsTestCase>;

TEST_P(BraveVpnApiClientGetTimezonesForRegionsTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  MockResponseFor<endpoints::GetTimezonesForRegions>(url_loader_factory_,
                                                     test_case.response);
  EXPECT_EQ(CallClientApi(&BraveVpnApiClient::GetTimezonesForRegions),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientGetTimezonesForRegionsTest,
    testing::ValuesIn(
        WithCommonUnrecoverableCases<GetTimezonesForRegionsTestCase>(
            WithCommonRawJsonCases<GetTimezonesForRegionsTestCase>({}))),
    [](const auto& info) { return info.param.test_name; });

struct GetHostnamesForRegionTestCase {
  std::string test_name;
  endpoints::GetHostnamesForRegion::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientGetHostnamesForRegionTest =
    BraveVpnApiClientTest<GetHostnamesForRegionTestCase>;

TEST_P(BraveVpnApiClientGetHostnamesForRegionTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  MockResponseFor<endpoints::GetHostnamesForRegion>(url_loader_factory_,
                                                    test_case.response);
  EXPECT_EQ(CallClientApi(&BraveVpnApiClient::GetHostnamesForRegion,
                          kTestRegion, kTestRegionPrecision),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientGetHostnamesForRegionTest,
    testing::ValuesIn(
        WithCommonUnrecoverableCases<GetHostnamesForRegionTestCase>(
            WithCommonRawJsonCases<GetHostnamesForRegionTestCase>({}))),
    [](const auto& info) { return info.param.test_name; });

struct GetProfileCredentialsTestCase {
  std::string test_name;
  endpoints::GetProfileCredentials::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientGetProfileCredentialsTest =
    BraveVpnApiClientTest<GetProfileCredentialsTestCase>;

TEST_P(BraveVpnApiClientGetProfileCredentialsTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  brave_account::endpoint_client::UrlReplacements url_replacements;
  url_replacements.SetHost(kTestHostname);
  MockResponseFor<endpoints::GetProfileCredentials>(
      url_loader_factory_, test_case.response, url_replacements);
  EXPECT_EQ(CallClientApi(&BraveVpnApiClient::GetProfileCredentials,
                          kTestHostname, kTestSubscriberCredential,
                          endpoints::TransportProtocol::kWireguard,
                          kTestPublicKey, kTestMultihopExitRegion),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientGetProfileCredentialsTest,
    testing::ValuesIn(
        WithCommonUnrecoverableCases<GetProfileCredentialsTestCase>(
            WithCommonRawJsonCases<GetProfileCredentialsTestCase>({}))),
    [](const auto& info) { return info.param.test_name; });

struct VerifyCredentialsTestCase {
  std::string test_name;
  endpoints::VerifyCredentials::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientVerifyCredentialsTest =
    BraveVpnApiClientTest<VerifyCredentialsTestCase>;

TEST_P(BraveVpnApiClientVerifyCredentialsTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  brave_account::endpoint_client::UrlReplacements url_replacements;
  url_replacements.SetHost(kTestHostname);
  url_replacements.SetPath(
      base::StrCat({endpoints::VerifyCredentials::URL().path(), "/",
                    kTestClientId, "/verify-credentials"}));
  MockResponseFor<endpoints::VerifyCredentials>(
      url_loader_factory_, test_case.response, url_replacements);
  EXPECT_EQ(CallClientApi(&BraveVpnApiClient::VerifyCredentials, kTestHostname,
                          kTestClientId, kTestApiAuthToken),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientVerifyCredentialsTest,
    testing::ValuesIn(WithCommonUnrecoverableCases<VerifyCredentialsTestCase>(
        WithCommonRawJsonCases<VerifyCredentialsTestCase>({}))),
    [](const auto& info) { return info.param.test_name; });

struct InvalidateCredentialsTestCase {
  std::string test_name;
  endpoints::InvalidateCredentials::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientInvalidateCredentialsTest =
    BraveVpnApiClientTest<InvalidateCredentialsTestCase>;

TEST_P(BraveVpnApiClientInvalidateCredentialsTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  brave_account::endpoint_client::UrlReplacements url_replacements;
  url_replacements.SetHost(kTestHostname);
  url_replacements.SetPath(
      base::StrCat({endpoints::InvalidateCredentials::URL().path(), "/",
                    kTestClientId, "/invalidate-credentials"}));
  MockResponseFor<endpoints::InvalidateCredentials>(
      url_loader_factory_, test_case.response, url_replacements);
  EXPECT_EQ(CallClientApi(&BraveVpnApiClient::InvalidateCredentials,
                          kTestHostname, kTestClientId, kTestApiAuthToken,
                          kTestSubscriberCredential),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientInvalidateCredentialsTest,
    testing::ValuesIn(
        WithCommonUnrecoverableCases<InvalidateCredentialsTestCase>(
            WithCommonRawJsonCases<InvalidateCredentialsTestCase>({}))),
    [](const auto& info) { return info.param.test_name; });

struct GetAvailableMultihopExitRegionsTestCase {
  std::string test_name;
  endpoints::GetAvailableMultihopExitRegions::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientGetAvailableMultihopExitRegionsTest =
    BraveVpnApiClientTest<GetAvailableMultihopExitRegionsTestCase>;

TEST_P(BraveVpnApiClientGetAvailableMultihopExitRegionsTest,
       MapsResponseToResult) {
  const auto& test_case = GetParam();
  brave_account::endpoint_client::UrlReplacements url_replacements;
  url_replacements.SetHost(kTestHostname);
  url_replacements.SetPath(
      base::StrCat({endpoints::GetAvailableMultihopExitRegions::URL().path(),
                    "/", kTestClientId, "/config/multihop"}));
  MockResponseFor<endpoints::GetAvailableMultihopExitRegions>(
      url_loader_factory_, test_case.response, url_replacements);
  EXPECT_EQ(CallClientApi(&BraveVpnApiClient::GetAvailableMultihopExitRegions,
                          kTestHostname, kTestClientId, kTestApiAuthToken),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientGetAvailableMultihopExitRegionsTest,
    testing::ValuesIn(
        WithCommonUnrecoverableCases<GetAvailableMultihopExitRegionsTestCase>(
            WithCommonRawJsonCases<GetAvailableMultihopExitRegionsTestCase>(
                {}))),
    [](const auto& info) { return info.param.test_name; });

struct SetMultihopExitRegionTestCase {
  std::string test_name;
  endpoints::SetMultihopExitRegion::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientSetMultihopExitRegionTest =
    BraveVpnApiClientTest<SetMultihopExitRegionTestCase>;

TEST_P(BraveVpnApiClientSetMultihopExitRegionTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  brave_account::endpoint_client::UrlReplacements url_replacements;
  url_replacements.SetHost(kTestHostname);
  url_replacements.SetPath(
      base::StrCat({endpoints::SetMultihopExitRegion::URL().path(), "/",
                    kTestClientId, "/config/multihop"}));
  MockResponseFor<endpoints::SetMultihopExitRegion>(
      url_loader_factory_, test_case.response, url_replacements);
  EXPECT_EQ(
      CallClientApi(&BraveVpnApiClient::SetMultihopExitRegion, kTestHostname,
                    kTestClientId, kTestApiAuthToken, kTestMultihopExitRegion),
      test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientSetMultihopExitRegionTest,
    testing::ValuesIn(
        WithCommonUnrecoverableCases<SetMultihopExitRegionTestCase>(
            WithCommonRawJsonCases<SetMultihopExitRegionTestCase>({}))),
    [](const auto& info) { return info.param.test_name; });

struct ClearMultihopExitRegionTestCase {
  std::string test_name;
  endpoints::SetMultihopExitRegion::Response response;
  base::expected<std::string, std::string> expected;
};

using BraveVpnApiClientClearMultihopExitRegionTest =
    BraveVpnApiClientTest<ClearMultihopExitRegionTestCase>;

TEST_P(BraveVpnApiClientClearMultihopExitRegionTest, MapsResponseToResult) {
  const auto& test_case = GetParam();
  brave_account::endpoint_client::UrlReplacements url_replacements;
  url_replacements.SetHost(kTestHostname);
  url_replacements.SetPath(
      base::StrCat({endpoints::SetMultihopExitRegion::URL().path(), "/",
                    kTestClientId, "/config/multihop"}));
  MockResponseFor<endpoints::SetMultihopExitRegion>(
      url_loader_factory_, test_case.response, url_replacements);
  EXPECT_EQ(CallClientApi(&BraveVpnApiClient::ClearMultihopExitRegion,
                          kTestHostname, kTestClientId, kTestApiAuthToken),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BraveVpnApiClientTests,
    BraveVpnApiClientClearMultihopExitRegionTest,
    testing::ValuesIn(
        WithCommonUnrecoverableCases<ClearMultihopExitRegionTestCase>(
            WithCommonRawJsonCases<ClearMultihopExitRegionTestCase>({}))),
    [](const auto& info) { return info.param.test_name; });

// Verifies request construction independent of the response, unlike
// BraveVpnApiClientTest<> parameterized suites, which verify response-to-result
// mapping independent of the request.
class BraveVpnApiClientRequestBodyTest : public BraveVpnApiClientTestBase {};

TEST_F(BraveVpnApiClientRequestBodyTest,
       ClearMultihopExitRegionSendsDisabledSentinel) {
  brave_account::endpoint_client::UrlReplacements url_replacements;
  url_replacements.SetHost(kTestHostname);
  url_replacements.SetPath(
      base::StrCat({endpoints::SetMultihopExitRegion::URL().path(), "/",
                    kTestClientId, "/config/multihop"}));

  std::optional<std::string> sent_body;
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        if (request.request_body) {
          const auto& elements = *request.request_body->elements();
          ASSERT_EQ(elements.size(), 1u);
          sent_body = std::string(
              elements[0].As<network::DataElementBytes>().AsStringPiece());
        }
      }));

  // The mocked response's content is irrelevant.
  MockResponseFor<endpoints::SetMultihopExitRegion>(
      url_loader_factory_,
      endpoints::SetMultihopExitRegion::Response{
          .net_error = net::OK,
          .status_code = net::HTTP_OK,
          .body = base::ok(endpoints::RawJsonResponseBody{.json = "{}"})},
      url_replacements);

  std::ignore = CallClientApi(&BraveVpnApiClient::ClearMultihopExitRegion,
                              kTestHostname, kTestClientId, kTestApiAuthToken);

  ASSERT_TRUE(sent_body.has_value());
  const auto parsed =
      base::JSONReader::ReadDict(*sent_body, base::JSON_PARSE_RFC);
  ASSERT_TRUE(parsed);
  EXPECT_EQ(*parsed->FindString("multihop-exit-region"), "disabled");
}

}  // namespace brave_vpn::v2
