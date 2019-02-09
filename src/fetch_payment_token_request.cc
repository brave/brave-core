/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "fetch_payment_token_request.h"
#include "ads_serve_helper.h"

namespace confirmations {

FetchPaymentTokenRequest::FetchPaymentTokenRequest() = default;

FetchPaymentTokenRequest::~FetchPaymentTokenRequest() = default;

// GET /v1/confirmation/{confirmation_id}/paymentToken

std::string FetchPaymentTokenRequest::BuildUrl(
    const std::string& confirmation_id) const {
  std::string endpoint = "/v1/confirmation/";
  endpoint += confirmation_id;
  endpoint += "/paymentToken";

  return helper::AdsServe::GetURL().append(endpoint);
}

URLRequestMethod FetchPaymentTokenRequest::GetMethod() const {
  return GET;
}

}  // namespace confirmations
