/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_UTIL_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace braveledger_uphold {

const char kUrlStaging[] = "https://sandbox.uphold.com";
const char kUrlProduction[] = "https://uphold.com";
const char kAPIUrlStaging[] = "https://api-sandbox.uphold.com";
const char kAPIUrlProduction[] = "https://api.uphold.com";
const char kClientIdStaging[] = "4c2b665ca060d912fec5c735c734859a06118cc8";
const char kClientIdProduction[] = "6d8d9473ed20be627f71ed46e207f40c004c5b1a";
const char kClientSecretStaging[] = "67bf87da096748c5bc1e195cfbdd59db006618a0";
const char kClientSecretProduction[] =
    "de1aa4196c8d4aa50c6bc1371734e3f57f781f72";
const char kFeeAddressStaging[] = "7ab330ae-b7a6-4845-8f07-baab68848e4e";
const char kFeeAddressProduction[] = "532857d5-ff8b-4ca7-b9ef-5b1e40178340";

std::string GetClientId();

std::string GetClientSecret();

std::string GetUrl();

std::string GetAPIUrl(const std::string& path);

std::string GetFeeAddress();

std::string ConvertToProbi(const std::string& amount);

std::string GetVerifyUrl(const std::string& state);

std::string GetAddUrl(const std::string& address);

std::string GetWithdrawUrl(const std::string& address);

std::string GetSecondStepVerify();

ledger::ExternalWalletPtr GetWallet(
      std::map<std::string, ledger::ExternalWalletPtr> wallets);

std::vector<std::string> RequestAuthorization(
      const std::string& token = "");

std::string GenerateRandomString(bool testing);

std::string GetAccountUrl();

}  // namespace braveledger_uphold

#endif  // BRAVELEDGER_UPHOLD_UPHOLD_UTIL_H_
