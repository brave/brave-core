/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/access_token_fetcher_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/task_environment.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::_;

namespace brave_sync {

namespace {

constexpr char kSyncServiceURL[] = "https://sync-test.brave.com";

constexpr char kPublicKey[] =
    "f58ca446f0c33ee7e8e9874466da442b2e764afd77ad46034bdff9e01f9b87d4";
constexpr char kPrivateKey[] =
    "b5abda6940984c5153a2ba3653f047f98dfb19e39c3e02f07c8bbb0bd8e8872ef58ca446"
    "f0c33ee7e8e9874466da442b2e764afd77ad46034bdff9e01f9b87d4";

constexpr char kValidTimestampResponse[] = R"(
    {
      "timestamp": "1588741616",
      "expires_in": 3600
    })";

constexpr char kTokenResponseNoTimestamp[] = R"(
    {
      "expires_in": 3600,
    })";

constexpr char kValidFailureTimestampResponse[] = R"(
    {
      "error": "invalid_grant"
    })";

class MockAccessTokenConsumer : public AccessTokenConsumer {
 public:
  MockAccessTokenConsumer() {}
  ~MockAccessTokenConsumer() override {}

  MOCK_METHOD1(OnGetTokenSuccess,
               void(const AccessTokenConsumer::TokenResponse&));
  MOCK_METHOD1(OnGetTokenFailure, void(const GoogleServiceAuthError& error));
};

class URLLoaderFactoryInterceptor {
 public:
  MOCK_METHOD1(Intercept, void(const network::ResourceRequest&));
};

MATCHER_P(resourceRequestUrlEquals, url, "") {
  return arg.url == url;
}
}  // namespace

class AccessTokenFetcherImplTest : public testing::Test {
 public:
  AccessTokenFetcherImplTest()
      : fetcher_(&consumer_,
                 url_loader_factory_.GetSafeWeakWrapper(),
                 GURL(kSyncServiceURL)) {
    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &URLLoaderFactoryInterceptor::Intercept,
        base::Unretained(&url_loader_factory_interceptor_)));
    base::HexStringToBytes(kPublicKey, &public_key_);
    base::HexStringToBytes(kPrivateKey, &private_key_);
    base::RunLoop().RunUntilIdle();
  }

  void SetupGetTimestamp(int net_error_code,
                           net::HttpStatusCode http_response_code,
                           const std::string& body) {
    GURL url = fetcher_.MakeGetTimestampUrl();
    if (net_error_code == net::OK) {
      url_loader_factory_.AddResponse(url.spec(), body, http_response_code);
    } else {
      url_loader_factory_.AddResponse(
          url, network::mojom::URLResponseHead::New(), body,
          network::URLLoaderCompletionStatus(net_error_code));
    }

    EXPECT_CALL(url_loader_factory_interceptor_,
                Intercept(resourceRequestUrlEquals(url)));
  }

  void SetupTimestampProxyError() {
    GURL url = fetcher_.MakeGetTimestampUrl();
    url_loader_factory_.AddResponse(
        url,
        network::CreateURLResponseHead(net::HTTP_PROXY_AUTHENTICATION_REQUIRED),
        std::string(),
        network::URLLoaderCompletionStatus(net::ERR_TUNNEL_CONNECTION_FAILED),
        network::TestURLLoaderFactory::Redirects(),
        network::TestURLLoaderFactory::kSendHeadersOnNetworkError);

    EXPECT_CALL(url_loader_factory_interceptor_,
                Intercept(resourceRequestUrlEquals(url)));
  }

  std::vector<uint8_t> GetPublicKey() { return public_key_; }
  std::vector<uint8_t> GetPrivateKey() { return private_key_; }

 protected:
  base::test::SingleThreadTaskEnvironment task_environment_;
  MockAccessTokenConsumer consumer_;
  URLLoaderFactoryInterceptor url_loader_factory_interceptor_;
  network::TestURLLoaderFactory url_loader_factory_;
  AccessTokenFetcherImpl fetcher_;
  std::vector<uint8_t> public_key_;
  std::vector<uint8_t> private_key_;
};

TEST_F(AccessTokenFetcherImplTest, GetTimestampRequestFailure) {
  SetupGetTimestamp(net::ERR_FAILED, net::HTTP_OK, std::string());
  EXPECT_CALL(consumer_, OnGetTokenFailure(_)).Times(1);
  fetcher_.Start(GetPublicKey(), GetPrivateKey());
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, GetTimestampResponseCodeFailure) {
  SetupGetTimestamp(net::OK, net::HTTP_FORBIDDEN, std::string());
  EXPECT_CALL(consumer_, OnGetTokenFailure(_)).Times(1);
  fetcher_.Start(GetPublicKey(), GetPrivateKey());
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, TimestampProxyFailure) {
  GoogleServiceAuthError expected_error =
      GoogleServiceAuthError::FromConnectionError(
          net::ERR_TUNNEL_CONNECTION_FAILED);
  ASSERT_TRUE(expected_error.IsTransientError());
  SetupTimestampProxyError();
  EXPECT_CALL(consumer_, OnGetTokenFailure(expected_error)).Times(1);
  fetcher_.Start(GetPublicKey(), GetPrivateKey());
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, AccessTokenSuccess) {
  SetupGetTimestamp(net::OK, net::HTTP_OK, kValidTimestampResponse);
  EXPECT_CALL(consumer_, OnGetTokenSuccess(_)).Times(1);
  fetcher_.Start(GetPublicKey(), GetPrivateKey());
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, ParseGetTimestampResponseNoBody) {
  std::string ts;
  int expires_in;
  auto empty_body = std::make_unique<std::string>("");
  EXPECT_FALSE(AccessTokenFetcherImpl::ParseGetTimestampSuccessResponse(
      std::move(empty_body), &ts, &expires_in));
  EXPECT_TRUE(ts.empty());
}

TEST_F(AccessTokenFetcherImplTest, ParseGetTimestampResponseBadJson) {
  std::string ts;
  int expires_in;
  EXPECT_FALSE(AccessTokenFetcherImpl::ParseGetTimestampSuccessResponse(
      std::make_unique<std::string>("foo"), &ts, &expires_in));
  EXPECT_TRUE(ts.empty());
}

TEST_F(AccessTokenFetcherImplTest, ParseGetTimestampResponseSuccess) {
  std::string ts;
  int expires_in;
  EXPECT_TRUE(AccessTokenFetcherImpl::ParseGetTimestampSuccessResponse(
      std::make_unique<std::string>(kValidTimestampResponse), &ts,
      &expires_in));
  EXPECT_EQ("1588741616", ts);
  EXPECT_EQ(3600, expires_in);
}

TEST_F(AccessTokenFetcherImplTest, ParseGetTimestampFailureInvalidError) {
  std::string error;
  EXPECT_FALSE(AccessTokenFetcherImpl::ParseGetTimestampFailureResponse(
      std::make_unique<std::string>(kTokenResponseNoTimestamp), &error));
  EXPECT_TRUE(error.empty());
}

TEST_F(AccessTokenFetcherImplTest, ParseGetTimestampFailure) {
  std::string error;
  EXPECT_TRUE(AccessTokenFetcherImpl::ParseGetTimestampFailureResponse(
      std::make_unique<std::string>(kValidFailureTimestampResponse), &error));
  EXPECT_EQ("invalid_grant", error);
}

}  // namespace brave_sync
