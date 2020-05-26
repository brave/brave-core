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

constexpr char kValidTokenResponse[] = R"(
    {
      "access_token": "at1",
      "expires_in": 3600,
      "id_token": "id_token"
    })";

constexpr char kTokenResponseNoAccessToken[] = R"(
    {
      "expires_in": 3600,
    })";

constexpr char kValidFailureTokenResponse[] = R"(
    {
      "error": "invalid_grant"
    })";

constexpr char kValidTimestampResponse[] = R"(
    {
      "timestamp": "1588741616"
    })";

class MockAccessTokenConsumer : public AccessTokenConsumer {
 public:
  MockAccessTokenConsumer() {}
  ~MockAccessTokenConsumer() override {}

  MOCK_METHOD1(OnGetTokenSuccess,
               void(const AccessTokenConsumer::TokenResponse&));
  MOCK_METHOD1(OnGetTokenFailure, void(const GoogleServiceAuthError& error));

  MOCK_METHOD1(OnGetTimestampSuccess, void(const std::string& ts));
  MOCK_METHOD1(OnGetTimestampFailure,
               void(const GoogleServiceAuthError& error));
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
                 GURL(kSyncServiceURL),
                 "refresh_token") {
    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &URLLoaderFactoryInterceptor::Intercept,
        base::Unretained(&url_loader_factory_interceptor_)));
    base::RunLoop().RunUntilIdle();
  }

  void SetupGetAccessToken(int net_error_code,
                           net::HttpStatusCode http_response_code,
                           const std::string& body) {
    GURL url = fetcher_.MakeGetAccessTokenUrl();
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

  void SetupAccessTokenProxyError() {
    GURL url = fetcher_.MakeGetAccessTokenUrl();
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

 protected:
  base::test::SingleThreadTaskEnvironment task_environment_;
  MockAccessTokenConsumer consumer_;
  URLLoaderFactoryInterceptor url_loader_factory_interceptor_;
  network::TestURLLoaderFactory url_loader_factory_;
  AccessTokenFetcherImpl fetcher_;
};

TEST_F(AccessTokenFetcherImplTest, GetAccessTokenRequestFailure) {
  SetupGetAccessToken(net::ERR_FAILED, net::HTTP_OK, std::string());
  EXPECT_CALL(consumer_, OnGetTokenFailure(_)).Times(1);
  fetcher_.Start("client_id", "client_secret", "timestamp");
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, GetTimestampRequestFailure) {
  SetupGetTimestamp(net::ERR_FAILED, net::HTTP_OK, std::string());
  EXPECT_CALL(consumer_, OnGetTimestampFailure(_)).Times(1);
  fetcher_.StartGetTimestamp();
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, GetAccessTokenResponseCodeFailure) {
  SetupGetAccessToken(net::OK, net::HTTP_FORBIDDEN, std::string());
  EXPECT_CALL(consumer_, OnGetTokenFailure(_)).Times(1);
  fetcher_.Start("client_id", "client_secret", "timestamp");
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, GetTimestampResponseCodeFailure) {
  SetupGetTimestamp(net::OK, net::HTTP_FORBIDDEN, std::string());
  EXPECT_CALL(consumer_, OnGetTimestampFailure(_)).Times(1);
  fetcher_.StartGetTimestamp();
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, AccessTokenProxyFailure) {
  GoogleServiceAuthError expected_error =
      GoogleServiceAuthError::FromConnectionError(
          net::ERR_TUNNEL_CONNECTION_FAILED);
  ASSERT_TRUE(expected_error.IsTransientError());
  SetupAccessTokenProxyError();
  EXPECT_CALL(consumer_, OnGetTokenFailure(expected_error)).Times(1);
  fetcher_.Start("client_id", "client_secret", "timestamp");
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, TimestampProxyFailure) {
  GoogleServiceAuthError expected_error =
      GoogleServiceAuthError::FromConnectionError(
          net::ERR_TUNNEL_CONNECTION_FAILED);
  ASSERT_TRUE(expected_error.IsTransientError());
  SetupTimestampProxyError();
  EXPECT_CALL(consumer_, OnGetTimestampFailure(expected_error)).Times(1);
  fetcher_.StartGetTimestamp();
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, AccessTokenSuccess) {
  SetupGetAccessToken(net::OK, net::HTTP_OK, kValidTokenResponse);
  EXPECT_CALL(consumer_, OnGetTokenSuccess(_)).Times(1);
  fetcher_.Start("client_id", "client_secret", "timestamp");
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, TimestampSuccess) {
  SetupGetTimestamp(net::OK, net::HTTP_OK, kValidTimestampResponse);
  EXPECT_CALL(consumer_, OnGetTimestampSuccess(_)).Times(1);
  fetcher_.StartGetTimestamp();
  base::RunLoop().RunUntilIdle();
}

TEST_F(AccessTokenFetcherImplTest, MakeGetAccessTokenBody) {
  std::string body =
      "client_id=cid1&"
      "client_secret=cs1&"
      "timestamp=1234&"
      "refresh_token=rt1";
  EXPECT_EQ(body, AccessTokenFetcherImpl::MakeGetAccessTokenBody(
                      "cid1", "cs1", "1234", "rt1"));
}

TEST_F(AccessTokenFetcherImplTest, ParseGetAccessTokenResponseNoBody) {
  std::string at;
  int expires_in;
  std::string id_token;
  auto empty_body = std::make_unique<std::string>("");
  EXPECT_FALSE(AccessTokenFetcherImpl::ParseGetAccessTokenSuccessResponse(
      std::move(empty_body), &at, &expires_in, &id_token));
  EXPECT_TRUE(at.empty());
}

TEST_F(AccessTokenFetcherImplTest, ParseGetTimestampResponseNoBody) {
  std::string ts;
  auto empty_body = std::make_unique<std::string>("");
  EXPECT_FALSE(AccessTokenFetcherImpl::ParseGetTimestampSuccessResponse(
      std::move(empty_body), &ts));
  EXPECT_TRUE(ts.empty());
}

TEST_F(AccessTokenFetcherImplTest, ParseGetAccessTokenResponseBadJson) {
  std::string at;
  int expires_in;
  std::string id_token;
  EXPECT_FALSE(AccessTokenFetcherImpl::ParseGetAccessTokenSuccessResponse(
      std::make_unique<std::string>("foo"), &at, &expires_in, &id_token));
  EXPECT_TRUE(at.empty());
}

TEST_F(AccessTokenFetcherImplTest, ParseGetTimestampResponseBadJson) {
  std::string ts;
  EXPECT_FALSE(AccessTokenFetcherImpl::ParseGetTimestampSuccessResponse(
      std::make_unique<std::string>("foo"), &ts));
  EXPECT_TRUE(ts.empty());
}

TEST_F(AccessTokenFetcherImplTest, ParseGetAccessTokenResponseSuccess) {
  std::string at;
  int expires_in;
  std::string id_token;
  EXPECT_TRUE(AccessTokenFetcherImpl::ParseGetAccessTokenSuccessResponse(
      std::make_unique<std::string>(kValidTokenResponse), &at, &expires_in,
      &id_token));
  EXPECT_EQ("at1", at);
  EXPECT_EQ(3600, expires_in);
  EXPECT_EQ("id_token", id_token);
}

TEST_F(AccessTokenFetcherImplTest, ParseGetTimestampResponseSuccess) {
  std::string ts;
  EXPECT_TRUE(AccessTokenFetcherImpl::ParseGetTimestampSuccessResponse(
      std::make_unique<std::string>(kValidTimestampResponse), &ts));
  EXPECT_EQ("1588741616", ts);
}

TEST_F(AccessTokenFetcherImplTest, ParseGetAccessTokenFailureInvalidError) {
  std::string error;
  EXPECT_FALSE(AccessTokenFetcherImpl::ParseGetAccessTokenFailureResponse(
      std::make_unique<std::string>(kTokenResponseNoAccessToken), &error));
  EXPECT_TRUE(error.empty());
}

TEST_F(AccessTokenFetcherImplTest, ParseGetAccessTokenFailure) {
  std::string error;
  EXPECT_TRUE(AccessTokenFetcherImpl::ParseGetAccessTokenFailureResponse(
      std::make_unique<std::string>(kValidFailureTokenResponse), &error));
  EXPECT_EQ("invalid_grant", error);
}

}  // namespace brave_sync
