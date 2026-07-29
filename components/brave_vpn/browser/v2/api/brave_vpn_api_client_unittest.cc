/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/brave_vpn_api_client.h"

#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/test_support.h"
#include "brave/components/brave_vpn/browser/v2/api/error_body.h"
#include "brave/components/brave_vpn/browser/v2/api/purchase_endpoints.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {
namespace {
using brave_account::endpoint_client::MockResponseFor;

constexpr char kTestSkusCredential[] = "test-skus-credential";
constexpr char kTestSubscriberCredential[] = "test-subscriber-credential";
constexpr char kTestEnvironment[] = "test-env";
}  // namespace

struct GetSubscriberCredentialV12TestCase {
  std::string test_name;
  endpoints::GetSubscriberCredentialV12::Response response;
  base::expected<std::string, std::string> expected;
};

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
    testing::Values(
        // A transport failure surfaces the net-error description, not an HTTP
        // status.
        GetSubscriberCredentialV12TestCase{
            .test_name = "TransportErrorReturnsNetErrorString",
            .response = {.net_error = net::ERR_TIMED_OUT},
            .expected =
                base::unexpected(net::ErrorToString(net::ERR_TIMED_OUT))},
        // A non-2xx response whose body is not a parseable error falls back to
        // the HTTP status description (the status_code branch of
        // MaybeDescribeUnrecoverableResponse).
        GetSubscriberCredentialV12TestCase{
            .test_name = "HttpErrorWithUnparseableBodyDescribesStatus",
            .response = {.net_error = net::OK,
                         .status_code = net::HTTP_INTERNAL_SERVER_ERROR},
            .expected = base::unexpected(absl::StrFormat(
                "HTTP %d %s: body missing or failed to parse",
                net::HTTP_INTERNAL_SERVER_ERROR,
                net::GetHttpReasonPhrase(net::HTTP_INTERNAL_SERVER_ERROR)))},
        // A 2xx response with a parseable success body yields the subscriber
        // credential.
        GetSubscriberCredentialV12TestCase{
            .test_name = "Success",
            .response = {.net_error = net::OK,
                         .status_code = net::HTTP_OK,
                         .body =
                             endpoints::GetSubscriberCredentialV12SuccessBody{
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
            .expected = base::unexpected("Token no longer valid.")}),
    [](const auto& info) { return info.param.test_name; });

}  // namespace brave_vpn::v2
