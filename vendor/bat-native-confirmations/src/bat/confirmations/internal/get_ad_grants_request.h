/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_GET_AD_GRANTS_REQUEST_H_
#define BAT_CONFIRMATIONS_INTERNAL_GET_AD_GRANTS_REQUEST_H_

#include <string>
#include <vector>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"

namespace confirmations {

class GetAdGrantsRequest {
 public:
  GetAdGrantsRequest();
  ~GetAdGrantsRequest();

  std::string BuildUrl(const WalletInfo& wallet_info) const;

  URLRequestMethod GetMethod() const;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_GET_AD_GRANTS_REQUEST_H_
