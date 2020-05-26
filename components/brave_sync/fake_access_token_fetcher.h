/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_FAKE_ACCESS_TOKEN_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_FAKE_ACCESS_TOKEN_FETCHER_H_

#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_sync/access_token_fetcher.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace brave_sync {
class FakeAccessTokenFetcher : public AccessTokenFetcher {
 public:
  explicit FakeAccessTokenFetcher(AccessTokenConsumer* consumer);
  ~FakeAccessTokenFetcher() override;

  void Start(const std::string& client_id,
                     const std::string& client_secret,
                     const std::string& timestamp) override;

  void StartGetTimestamp() override;

  // Cancels the current request and informs the consumer.
  void CancelRequest() override;
  void SetAccessTokenResponseForTest(
      const AccessTokenConsumer::TokenResponse& token_response) override;

  void SetTokenResponseCallback(base::OnceClosure on_available);
  void SetTokenResponseError(const GoogleServiceAuthError& error);

 private:
  void OnGetTokenSuccess(
    const AccessTokenConsumer::TokenResponse& token_response);
  void OnGetTokenFailure(const GoogleServiceAuthError& error);

  AccessTokenConsumer::TokenResponse pending_response_;
  // Initial state is NONE
  GoogleServiceAuthError pending_error_;

  base::OnceClosure on_available_;

  base::WeakPtrFactory<FakeAccessTokenFetcher> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(FakeAccessTokenFetcher);
};
}   // namespace brave_sync
#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_FAKE_ACCESS_TOKEN_FETCHER_H_
