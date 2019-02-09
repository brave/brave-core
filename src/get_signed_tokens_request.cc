/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "get_signed_tokens_request.h"
#include "ads_serve_helper.h"

namespace confirmations {

GetSignedTokensRequest::GetSignedTokensRequest() = default;

GetSignedTokensRequest::~GetSignedTokensRequest() = default;

// GET /v1/confirmation/token/{payment_id}?nonce={nonce}

std::string GetSignedTokensRequest::BuildUrl(
    const WalletInfo& wallet_info,
    const std::string& nonce) const {
  std::string endpoint = "/v1/confirmation/token/";
  endpoint += wallet_info.payment_id;
  endpoint += "?nonce=";
  endpoint += nonce;

  return helper::AdsServe::GetURL().append(endpoint);
}

URLRequestMethod GetSignedTokensRequest::GetMethod() const {
  return GET;
}

}  // namespace confirmations
