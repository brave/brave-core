/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/access_token_consumer.h"

namespace brave_sync {

class AccessTokenFetcher {
 public:
  explicit AccessTokenFetcher(AccessTokenConsumer* consumer);
  virtual ~AccessTokenFetcher();

  virtual void Start(const std::vector<uint8_t>& public_key,
                     const std::vector<uint8_t>& private_key) = 0;

  // Cancels the current request and informs the consumer.
  virtual void CancelRequest() = 0;

  void SetAccessTokenConsumerForTest(AccessTokenConsumer* consumer);
  virtual void SetAccessTokenResponseForTest(
      const AccessTokenConsumer::TokenResponse& token_response);

 protected:
  // Fires |OnGetTokenSuccess| on |consumer_|.
  void FireOnGetTokenSuccess(
      const AccessTokenConsumer::TokenResponse& token_response);

  // Fires |OnGetTokenFailure| on |consumer_|.
  void FireOnGetTokenFailure(const GoogleServiceAuthError& error);

 private:
  AccessTokenConsumer* consumer_;

  DISALLOW_COPY_AND_ASSIGN(AccessTokenFetcher);
};

}   // namespace brave_sync
#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_ACCESS_TOKEN_FETCHER_H_
