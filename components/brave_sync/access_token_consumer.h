/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_CONSUMER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_CONSUMER_H_

#include <string>

#include "base/time/time.h"

class GoogleServiceAuthError;

namespace brave_sync {

class AccessTokenConsumer {
 public:
  // Structure representing information contained in access token.
  struct TokenResponse {
    TokenResponse() = default;
    TokenResponse(const std::string& access_token,
                  const base::Time& expiration_time);
    TokenResponse(const TokenResponse& response);

    // access token.
    std::string access_token;

    // The date until which the |access_token| can be used.
    // This value has a built-in safety margin, so it can be used as-is.
    base::Time expiration_time;
  };

  AccessTokenConsumer() = default;
  virtual ~AccessTokenConsumer();

  // Success callback.
  virtual void OnGetTokenSuccess(const TokenResponse& token_response);

  // Failure callback.
  virtual void OnGetTokenFailure(const GoogleServiceAuthError& error);

  DISALLOW_COPY_AND_ASSIGN(AccessTokenConsumer);
};

}   // namespace brave_sync
#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_CONSUMER_H_
