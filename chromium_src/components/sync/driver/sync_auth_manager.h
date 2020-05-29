/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_AUTH_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_AUTH_MANAGER_H_

#include "brave/components/brave_sync/access_token_consumer.h"
#include "brave/components/brave_sync/access_token_fetcher_impl.h"
#include "components/signin/public/identity_manager/access_token_info.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#define BRAVE_SYNC_AUTH_MANAGER_H_                                          \
  void CreateAccessTokenFetcher(                                            \
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,    \
      const GURL& sync_service_url);                                        \
  void SetAccessTokenFetcherForTest(                                        \
      std::unique_ptr<brave_sync::AccessTokenFetcher> fetcher);             \
  brave_sync::AccessTokenFetcher* GetAccessTokenFetcherForTest();           \
  void DeriveSigningKeys(const std::string& seed);                          \
  void ResetKeys();                                                         \
  void OnGetTokenSuccess(                                                   \
      const brave_sync::AccessTokenConsumer::TokenResponse& token_response) \
      override;                                                             \
  void OnGetTokenFailure(const GoogleServiceAuthError& error) override;     \
                                                                            \
 private:                                                                   \
  void AccessTokenFetchedDoNothing(                                         \
      GoogleServiceAuthError error,                                         \
      signin::AccessTokenInfo access_token_info) {}                         \
  std::vector<uint8_t> public_key_;                                         \
  std::vector<uint8_t> private_key_;                                        \
  std::unique_ptr<brave_sync::AccessTokenFetcher> access_token_fetcher_;
#include "../../../../../components/sync/driver/sync_auth_manager.h"
#undef BRAVE_SYNC_AUTH_MANAGER_H_
#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_AUTH_MANAGER_H_
