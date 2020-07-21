/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/request_promotion.h"
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

std::string ClaimCredsUrl(const std::string& promotion_id) {
    const std::string& path = base::StringPrintf(
      "/promotions/%s",
      promotion_id.c_str());
  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

std::string FetchSignedCredsUrl(
    const std::string& promotion_id,
    const std::string& claim_id) {
    const std::string& path = base::StringPrintf(
      "/promotions/%s/claims/%s",
      promotion_id.c_str(),
      claim_id.c_str());
  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

std::string GetRedeemTokensUrl() {
  return BuildUrl("/suggestions", PREFIX_V1, ServerTypes::kPromotion);
}

std::string ReportClobberedClaimsUrl() {
  return BuildUrl(
      "/promotions/reportclobberedclaims",
      PREFIX_V2,
      ServerTypes::kPromotion);
}

std::string GetTransferTokens() {
  return BuildUrl("/suggestions/claim", PREFIX_V1, ServerTypes::kPromotion);
}

std::string GetBatlossURL(
    const std::string& payment_id,
    const int32_t version) {
  const std::string& path = base::StringPrintf(
      "/wallets/%s/events/batloss/%d",
      payment_id.c_str(),
      version);
  return BuildUrl(path, PREFIX_V1, ServerTypes::kPromotion);
}

std::string GetCreateWalletURL() {
  return BuildUrl("/wallet/brave", PREFIX_V3, ServerTypes::kPromotion);
}

std::string GetRecoverWalletURL(const std::string& public_key) {
    const std::string& path = base::StringPrintf(
      "/wallet/recover/%s",
      public_key.c_str());
  return BuildUrl(path, PREFIX_V3, ServerTypes::kPromotion);
}

std::string GetClaimWalletURL(const std::string& payment_id) {
    const std::string& path = base::StringPrintf(
      "/wallet/uphold/%s/claim",
      payment_id.c_str());
  return BuildUrl(path, PREFIX_V3, ServerTypes::kPromotion);
}

std::string GetBalanceWalletURL(const std::string& payment_id) {
    const std::string path = base::StringPrintf(
      "/wallet/uphold/%s",
      payment_id.c_str());
  return BuildUrl(path, PREFIX_V3, ServerTypes::kPromotion);
}

}  // namespace braveledger_request_util
