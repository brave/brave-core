/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "crypto/random.h"

namespace braveledger_uphold {

std::string GetClientId() {
  return ledger::is_production
      ? kClientIdProduction
      : kClientIdStaging;
}

std::string GetClientSecret() {
  return ledger::is_production
      ? kClientSecretProduction
      : kClientSecretStaging;
}

std::string GetUrl() {
  return ledger::is_production
      ? kUrlProduction
      : kUrlStaging;
}

std::string GetAPIUrl(const std::string& path) {
  std::string url;
  if (ledger::is_production) {
    url = kAPIUrlProduction;
  } else {
    url = kAPIUrlStaging;
  }

  return url + path;
}

std::string GetFeeAddress() {
  return ledger::is_production
      ? kFeeAddressProduction
      : kFeeAddressStaging;
}

std::string ConvertToProbi(const std::string& amount) {
  if (amount.empty()) {
    return "0";
  }

  auto vec = base::SplitString(
      amount, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  const std::string probi = "000000000000000000";

  if (vec.size() == 1) {
    return vec.at(0) + probi;
  }

  const auto before_dot = vec.at(0);
  const auto after_dot = vec.at(1);
  const auto rest_probi = probi.substr(after_dot.size());

  return before_dot + after_dot + rest_probi;
}

std::string GetVerifyUrl(const std::string& state) {
  const std::string id = GetClientId();

  const std::string url = GetUrl();

  return base::StringPrintf(
      "%s/authorize/%s"
      "?scope=cards:read cards:write user:read transactions:read "
      "transactions:transfer:application "
      "transactions:transfer:others&intention=kyc&"
      "state=%s",
      url.c_str(),
      id.c_str(),
      state.c_str());
}

std::string GetAddUrl(const std::string& address) {
  const std::string url = GetUrl();

  if (address.empty()) {
    return "";
  }

  return base::StringPrintf(
      "%s/dashboard/cards/%s/add",
      url.c_str(),
      address.c_str());
}

std::string GetWithdrawUrl(const std::string& address) {
  const std::string url = GetUrl();

  if (address.empty()) {
    return "";
  }

  return base::StringPrintf(
      "%s/dashboard/cards/%s/use",
      url.c_str(),
      address.c_str());
}

std::string GetSecondStepVerify() {
  const std::string url = GetUrl();
  const std::string id = GetClientId();

  return base::StringPrintf(
      "%s/signup/step2?application_id=%s&intention=kyc",
      url.c_str(),
      id.c_str());
}

ledger::ExternalWalletPtr GetWallet(
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  for (auto& wallet : wallets) {
    if (wallet.first == ledger::kWalletUphold) {
      return std::move(wallet.second);
    }
  }

  return nullptr;
}

std::vector<std::string> RequestAuthorization(
    const std::string& token) {
  std::vector<std::string> headers;

  if (!token.empty()) {
    headers.push_back("Authorization: Bearer " + token);
    return headers;
  }

  const std::string id = GetClientId();
  const std::string secret = GetClientSecret();

  std::string user;
  base::Base64Encode(base::StringPrintf("%s:%s", id.c_str(), secret.c_str()),
                     &user);

  headers.push_back("Authorization: Basic " + user);

  return headers;
}

std::string GenerateRandomString(bool testing) {
  if (testing) {
    return "123456789";
  }

  const size_t kLength = 32;
  uint8_t bytes[kLength];
  crypto::RandBytes(bytes, sizeof(bytes));
  return base::HexEncode(bytes, sizeof(bytes));
}

std::string GetAccountUrl() {
  const std::string url = GetUrl();

  return base::StringPrintf(
      "%s/dashboard",
      url.c_str());
}

}  // namespace braveledger_uphold
