/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/access_token_fetcher.h"

namespace brave_sync {

AccessTokenFetcher::AccessTokenFetcher(AccessTokenConsumer* consumer)
    : consumer_(consumer) {}

AccessTokenFetcher::~AccessTokenFetcher() {}

void AccessTokenFetcher::SetAccessTokenConsumerForTest(
    AccessTokenConsumer* consumer) {
  consumer_ = consumer;
}

void AccessTokenFetcher::SetAccessTokenResponseForTest(
    const AccessTokenConsumer::TokenResponse& token_response) {}

void AccessTokenFetcher::FireOnGetTokenSuccess(
    const AccessTokenConsumer::TokenResponse& token_response) {
  consumer_->OnGetTokenSuccess(token_response);
}

void AccessTokenFetcher::FireOnGetTokenFailure(
    const GoogleServiceAuthError& error) {
  consumer_->OnGetTokenFailure(error);
}

void AccessTokenFetcher::FireOnGetTimestampSuccess(const std::string& ts) {
  consumer_->OnGetTimestampSuccess(ts);
}

void AccessTokenFetcher::FireOnGetTimestampFailure(
    const GoogleServiceAuthError& error) {
  consumer_->OnGetTimestampFailure(error);
}

}  // namespace brave_sync
