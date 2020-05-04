/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/get_ad_grants_request.h"
#include "bat/confirmations/internal/grant_rewards_serve_helper.h"

namespace confirmations {

GetAdGrantsRequest::GetAdGrantsRequest() = default;

GetAdGrantsRequest::~GetAdGrantsRequest() = default;

// GET GET /v1/promotions/ads/grants/summary?paymentId={payment_id}

std::string GetAdGrantsRequest::BuildUrl(
    const WalletInfo& wallet_info) const {
  DCHECK(!wallet_info.payment_id.empty());

  std::string endpoint = "/v1/promotions/ads/grants/summary?paymentId=";
  endpoint += wallet_info.payment_id;

  return helper::GrantRewardsServe::GetURL().append(endpoint);
}

URLRequestMethod GetAdGrantsRequest::GetMethod() const {
  return URLRequestMethod::GET;
}

}  // namespace confirmations
