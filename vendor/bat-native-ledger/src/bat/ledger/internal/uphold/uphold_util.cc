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
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "crypto/random.h"

namespace ledger {
namespace uphold {

std::string GetClientId() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? BUILDFLAG(UPHOLD_CLIENT_ID)
             : BUILDFLAG(UPHOLD_STAGING_CLIENT_ID);
}

std::string GetClientSecret() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? BUILDFLAG(UPHOLD_CLIENT_SECRET)
             : BUILDFLAG(UPHOLD_STAGING_CLIENT_SECRET);
}

std::string GetUrl() {
  return ledger::_environment == type::Environment::PRODUCTION ? kUrlProduction
                                                               : kUrlStaging;
}

std::string GetFeeAddress() {
  return ledger::_environment == type::Environment::PRODUCTION
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
      "transactions:transfer:application "
      "transactions:transfer:others"
      "&intention=login&"
      "state=%s",
      url.c_str(), id.c_str(), state.c_str());
}

std::string GetAddUrl(const std::string& address) {
  const std::string url = GetUrl();

  if (address.empty()) {
    return "";
  }

  return base::StringPrintf("%s/dashboard/cards/%s/add", url.c_str(),
                            address.c_str());
}

std::string GetWithdrawUrl(const std::string& address) {
  const std::string url = GetUrl();

  if (address.empty()) {
    return "";
  }

  return base::StringPrintf("%s/dashboard/cards/%s/use", url.c_str(),
                            address.c_str());
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

type::ExternalWalletPtr GenerateLinks(type::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  CheckWalletState(wallet.get());

  switch (wallet->status) {
    case type::WalletStatus::VERIFIED: {
      wallet->add_url = GetAddUrl(wallet->address);
      wallet->withdraw_url = GetWithdrawUrl(wallet->address);
      break;
    }
    case type::WalletStatus::PENDING:
    case type::WalletStatus::NOT_CONNECTED:
    case type::WalletStatus::DISCONNECTED_VERIFIED: {
      wallet->add_url = "";
      wallet->withdraw_url = "";
      break;
    }
    default:
      NOTREACHED();
  }

  wallet->account_url = GetAccountUrl();
  wallet->login_url = GetLoginUrl(wallet->one_time_string);
  wallet->activity_url = GetActivityUrl(wallet->address);

  return wallet;
}

template <typename T, typename... Ts>
bool one_of(T&& t, Ts&&... ts) {
  bool match = false;

  static_cast<void>(std::initializer_list<bool>{
      (match = match || std::forward<T>(t) == std::forward<Ts>(ts))...});

  return match;
}

void OnWalletStatusChange(LedgerImpl* ledger,
                          absl::optional<type::WalletStatus> from,
                          type::WalletStatus to) {
  DCHECK(ledger);
  DCHECK(!from ||
         one_of(*from, type::WalletStatus::NOT_CONNECTED,
                type::WalletStatus::DISCONNECTED_VERIFIED,
                type::WalletStatus::PENDING, type::WalletStatus::VERIFIED));
  DCHECK(one_of(to, type::WalletStatus::NOT_CONNECTED,
                type::WalletStatus::DISCONNECTED_VERIFIED,
                type::WalletStatus::PENDING, type::WalletStatus::VERIFIED));

  std::ostringstream oss{};
  if (from) {
    oss << *from << ' ';
  }
  oss << "==> " << to;

  ledger->database()->SaveEventLog(log::kWalletStatusChange, oss.str());
}

void CheckWalletState(const type::ExternalWallet* wallet) {
  if (!wallet)
    return;

  switch (wallet->status) {
    case type::WalletStatus::NOT_CONNECTED:
    case type::WalletStatus::DISCONNECTED_VERIFIED: {
      DCHECK(wallet->token.empty());
      DCHECK(wallet->address.empty());
      break;
    }
    case type::WalletStatus::PENDING: {
      DCHECK(!wallet->token.empty());
      DCHECK(wallet->address.empty());
      break;
    }
    case type::WalletStatus::VERIFIED: {
      DCHECK(!wallet->token.empty());
      DCHECK(!wallet->address.empty());
      break;
    }
    default:
      NOTREACHED() << " Unexpected Uphold wallet status " << wallet->status;
  }
}

}  // namespace uphold
}  // namespace ledger
