/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/buildflags.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "crypto/random.h"

namespace ledger {
namespace uphold {

std::string GetClientId() {
  return ledger::_environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(UPHOLD_CLIENT_ID)
             : BUILDFLAG(UPHOLD_STAGING_CLIENT_ID);
}

std::string GetClientSecret() {
  return ledger::_environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(UPHOLD_CLIENT_SECRET)
             : BUILDFLAG(UPHOLD_STAGING_CLIENT_SECRET);
}

std::string GetUrl() {
  return ledger::_environment == mojom::Environment::PRODUCTION ? kUrlProduction
                                                                : kUrlStaging;
}

std::string GetFeeAddress() {
  return ledger::_environment == mojom::Environment::PRODUCTION
             ? kFeeAddressProduction
             : kFeeAddressStaging;
}

std::string GetLoginUrl(const std::string& state) {
  const std::string id = GetClientId();
  const std::string url = GetUrl();

  return base::StringPrintf(
      "%s/authorize/%s"
      "?scope="
      "cards:read "
      "cards:write "
      "user:read "
      "transactions:read "
      "transactions:transfer:application "
      "transactions:transfer:others"
      "&intention=login&"
      "state=%s",
      url.c_str(), id.c_str(), state.c_str());
}

std::string GetAccountUrl() {
  const std::string url = GetUrl();

  return base::StringPrintf("%s/dashboard", url.c_str());
}

std::string GetActivityUrl(const std::string& address) {
  std::string url;

  if (!address.empty()) {
    url = base::StringPrintf("%s/dashboard/cards/%s/activity", GetUrl().c_str(),
                             address.c_str());
  }

  return url;
}

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  wallet->account_url = GetAccountUrl();
  wallet->activity_url = "";
  wallet->login_url = GetLoginUrl(wallet->one_time_string);

  if (wallet->status == mojom::WalletStatus::kConnected) {
    wallet->activity_url = GetActivityUrl(wallet->address);
  }

  return wallet;
}

}  // namespace uphold
}  // namespace ledger
