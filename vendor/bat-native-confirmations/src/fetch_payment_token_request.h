/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_FETCH_PAYMENT_TOKEN_REQUEST_H_
#define BAT_CONFIRMATIONS_FETCH_PAYMENT_TOKEN_REQUEST_H_

#include <string>

#include "bat/confirmations/confirmations_client.h"

namespace confirmations {

class FetchPaymentTokenRequest {
 public:
  FetchPaymentTokenRequest();
  ~FetchPaymentTokenRequest();

  std::string BuildUrl(
      const std::string& confirmation_id) const;

  URLRequestMethod GetMethod() const;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_FETCH_PAYMENT_TOKEN_REQUEST_H_
