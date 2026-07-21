/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/brave_vpn_api_client.h"

#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_vpn/browser/v2/api/purchase_endpoints.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

namespace brave_vpn::v2 {
namespace {
constexpr char kTestSkusCredential[] = "test-skus-credential";
constexpr char kTestSubscriberCredential[] = "test-subscriber-credential";
constexpr char kTestEnvironment[] = "test-env";
}  // namespace

class BraveVpnApiClientTest : public testing::Test {
 public:
  BraveVpnApiClientTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)),
        client_(shared_url_loader_factory_) {}

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
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  BraveVpnApiClient client_;
};

// A transport failure surfaces the net-error description, not an HTTP status.
TEST_F(BraveVpnApiClientTest, TransportErrorReturnsNetErrorString) {
  url_loader_factory_.AddResponse(
      endpoints::GetSubscriberCredentialV12::URL(),
      network::mojom::URLResponseHead::New(),
      /*content=*/std::string(),
      network::URLLoaderCompletionStatus(net::ERR_TIMED_OUT));

  const auto result =
      CallClientApi(&BraveVpnApiClient::GetSubscriberCredentialV12,
                    kTestSkusCredential, kTestEnvironment);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), net::ErrorToString(net::ERR_TIMED_OUT));
}

// A non-2xx response whose body is not a parseable error falls back to the HTTP
// status description (the status_code branch of DescribeTransportFailure).
TEST_F(BraveVpnApiClientTest, HttpErrorWithUnparseableBodyDescribesStatus) {
  url_loader_factory_.AddResponse(
      endpoints::GetSubscriberCredentialV12::URL().spec(), "not json at all",
      net::HTTP_INTERNAL_SERVER_ERROR);

  const auto result =
      CallClientApi(&BraveVpnApiClient::GetSubscriberCredentialV12,
                    kTestSkusCredential, kTestEnvironment);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            absl::StrFormat("Unexpected HTTP status: %d",
                            static_cast<int>(net::HTTP_INTERNAL_SERVER_ERROR)));
}

// API-specific tests.

TEST_F(BraveVpnApiClientTest, GetSubscriberCredentialV12_Success) {
  url_loader_factory_.AddResponse(
      endpoints::GetSubscriberCredentialV12::URL().spec(),
      absl::StrFormat(R"({"subscriber-credential":"%s"})",
                      kTestSubscriberCredential),
      net::HTTP_OK);

  const auto result =
      CallClientApi(&BraveVpnApiClient::GetSubscriberCredentialV12,
                    kTestSkusCredential, kTestEnvironment);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), kTestSubscriberCredential);
}

TEST_F(BraveVpnApiClientTest, GetSubscriberCredentialV12_RequestError) {
  url_loader_factory_.AddResponse(
      endpoints::GetSubscriberCredentialV12::URL().spec(),
      R"({"error-title":"Token no longer valid.","error-message":"gone"})",
      net::HTTP_BAD_REQUEST);

  const auto result =
      CallClientApi(&BraveVpnApiClient::GetSubscriberCredentialV12,
                    kTestSkusCredential, kTestEnvironment);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Token no longer valid.");
}

}  // namespace brave_vpn::v2
