/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/promotion_requests.h"
#include "bat/ledger/internal/request/request_util.h"

namespace braveledger_request_util {

std::string GetFetchPromotionUrl(
    const std::string& payment_id,
    const std::string& platform) {
  const std::string& arguments = base::StringPrintf(
      "migrate=true&paymentId=%s&platform=%s",
      payment_id.c_str(),
      platform.c_str());

  const std::string& path = base::StringPrintf(
      "/promotions?%s",
      arguments.c_str());

  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

std::string ClaimTokensUrl(const std::string& promotion_id) {
    const std::string& path = base::StringPrintf(
      "/promotions/%s",
      promotion_id.c_str());
  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

std::string FetchSignedTokensUrl(
    const std::string& promotion_id,
    const std::string& claim_id) {
    const std::string& path = base::StringPrintf(
      "/promotions/%s/claims/%s",
      promotion_id.c_str(),
      claim_id.c_str());
  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

std::string GetReedemSuggestionsUrl() {
  return BuildUrl("/suggestions", PREFIX_V1, ServerTypes::kPromotion);
}

std::string ReportClobberedClaimsUrl() {
  return BuildUrl(
      "/promotions/reportclobberedclaims",
      PREFIX_V1,
      ServerTypes::kPromotion);
}

}  // namespace braveledger_request_util
