/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "brave/components/brave_sync/access_token_consumer.h"
#include "brave/components/brave_sync/access_token_fetcher.h"
#include "url/gurl.h"

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_sync {

class AccessTokenFetcherImplTest;

class AccessTokenFetcherImpl : public AccessTokenFetcher {
 public:
  AccessTokenFetcherImpl(
      AccessTokenConsumer* consumer,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const GURL& sync_service_url,
      const std::string& refresh_token);
  ~AccessTokenFetcherImpl() override;

  // Implementation of AccessTokenFetcher
  void Start(const std::string& client_id,
             const std::string& client_secret,
             const std::string& timestamp) override;

  void StartGetTimestamp() override;

  void CancelRequest() override;

 private:
  void OnURLLoadComplete(std::unique_ptr<std::string> response_body);
  void OnTimestampLoadComplete(std::unique_ptr<std::string> response_body);

  // Helper methods for the flow.
  void StartGetAccessToken();
  void EndGetAccessToken(std::unique_ptr<std::string> response_body);

  // Helper mehtods for reporting back results.
  void OnGetTokenSuccess(
      const AccessTokenConsumer::TokenResponse& token_response);
  void OnGetTokenFailure(const GoogleServiceAuthError& error);

  bool IsNetFailure(network::SimpleURLLoader* loader, int* histogram_value);

  // Other helpers.
  GURL MakeGetAccessTokenUrl();
  GURL MakeGetTimestampUrl();
  static std::string MakeGetAccessTokenBody(const std::string& client_id,
                                            const std::string& client_secret,
                                            const std::string& timestamp,
                                            const std::string& refresh_token);

  static bool ParseGetAccessTokenSuccessResponse(
      std::unique_ptr<std::string> response_body,
      std::string* access_token,
      int* expires_in,
      std::string* id_token);

  static bool ParseGetAccessTokenFailureResponse(
      std::unique_ptr<std::string> response_body,
      std::string* error);

  static bool ParseGetTimestampSuccessResponse(
      std::unique_ptr<std::string> response_body,
      std::string* timestamp);

  // State that is set during construction.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  GURL sync_service_url_;
  const std::string refresh_token_;

  // While a fetch is in progress.
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  std::unique_ptr<network::SimpleURLLoader> ts_url_loader_;
  std::string client_id_;
  std::string client_secret_;
  std::string timestamp_;

  friend class AccessTokenFetcherImplTest;
  FRIEND_TEST_ALL_PREFIXES(AccessTokenFetcherImplTest, MakeGetAccessTokenBody);
  FRIEND_TEST_ALL_PREFIXES(AccessTokenFetcherImplTest,
                           ParseGetAccessTokenResponseNoBody);
  FRIEND_TEST_ALL_PREFIXES(AccessTokenFetcherImplTest,
                           ParseGetTimestampResponseNoBody);
  FRIEND_TEST_ALL_PREFIXES(AccessTokenFetcherImplTest,
                           ParseGetAccessTokenResponseBadJson);
  FRIEND_TEST_ALL_PREFIXES(AccessTokenFetcherImplTest,
                           ParseGetTimestampResponseBadJson);
  FRIEND_TEST_ALL_PREFIXES(AccessTokenFetcherImplTest,
                           ParseGetAccessTokenResponseSuccess);
  FRIEND_TEST_ALL_PREFIXES(AccessTokenFetcherImplTest,
                           ParseGetTimestampResponseSuccess);
  FRIEND_TEST_ALL_PREFIXES(AccessTokenFetcherImplTest,
                           ParseGetAccessTokenFailureInvalidError);
  FRIEND_TEST_ALL_PREFIXES(AccessTokenFetcherImplTest,
                           ParseGetAccessTokenFailure);

  DISALLOW_COPY_AND_ASSIGN(AccessTokenFetcherImpl);
};
}  // namespace brave_sync
#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_IMPL_H_
