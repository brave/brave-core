/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "crypto/random.h"

namespace ledger {
namespace uphold {

namespace {
bool g_show_newly_verified_wallet = true;
}  // namespace

std::string GetClientId() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? UPHOLD_CLIENT_ID
             : UPHOLD_STAGING_CLIENT_ID;
}

std::string GetClientSecret() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? UPHOLD_CLIENT_SECRET
             : UPHOLD_STAGING_CLIENT_SECRET;
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

std::string GetACAddress() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? kACAddressProduction
             : kACAddressStaging;
}

std::string GetAuthorizeUrl(const std::string& state, const bool kyc_flow) {
  const std::string id = GetClientId();
  const std::string intention = kyc_flow ? "kyc" : "login";
  const std::string url = GetUrl();

  return base::StringPrintf(
      "%s/authorize/%s"
      "?scope="
      "accounts:read "
      "accounts:write "
      "cards:read "
      "cards:write "
      "user:read "
      "transactions:deposit "
      "transactions:read "
      "transactions:transfer:application "
      "transactions:transfer:others"
      "&intention=%s&"
      "state=%s",
      url.c_str(), id.c_str(), intention.c_str(), state.c_str());
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

std::string GetSecondStepVerify() {
  const std::string url = GetUrl();
  const std::string id = GetClientId();

  return base::StringPrintf("%s/signup/step2?application_id=%s&intention=kyc",
                            url.c_str(), id.c_str());
}

std::string GetAccountUrl() {
  const std::string url = GetUrl();

  return base::StringPrintf("%s/dashboard", url.c_str());
}

type::ExternalWalletPtr GenerateLinks(type::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  switch (wallet->status) {
    case type::WalletStatus::VERIFIED: {
      DCHECK(!wallet->token.empty());
      DCHECK(!wallet->address.empty());
      wallet->add_url = GetAddUrl(wallet->address);
      wallet->withdraw_url = GetWithdrawUrl(wallet->address);
      break;
    }
    case type::WalletStatus::PENDING: {
      DCHECK(!wallet->token.empty());
      DCHECK(wallet->address.empty());
      wallet->add_url = GetSecondStepVerify();
      wallet->withdraw_url = GetSecondStepVerify();
      break;
    }
    case type::WalletStatus::NOT_CONNECTED:
    case type::WalletStatus::DISCONNECTED_VERIFIED: {
      DCHECK(wallet->token.empty());
      DCHECK(wallet->address.empty());
      wallet->add_url = "";
      wallet->withdraw_url = "";
      break;
    }
    default:
      NOTREACHED();
  }

  wallet->verify_url = GenerateVerifyLink(wallet->Clone());
  wallet->account_url = GetAccountUrl();
  wallet->login_url = GetAuthorizeUrl(wallet->one_time_string, false);

  return wallet;
}

std::string GenerateVerifyLink(type::ExternalWalletPtr wallet) {
  std::string url = "";
  if (!wallet) {
    return url;
  }

  switch (wallet->status) {
    case type::WalletStatus::VERIFIED:
      DCHECK(!wallet->token.empty());
      DCHECK(!wallet->address.empty());
      break;
    case type::WalletStatus::PENDING: {
      DCHECK(!wallet->token.empty());
      DCHECK(wallet->address.empty());
      url = GetSecondStepVerify();
      break;
    }
    case type::WalletStatus::NOT_CONNECTED:
    case type::WalletStatus::DISCONNECTED_VERIFIED: {
      DCHECK(wallet->token.empty());
      DCHECK(wallet->address.empty());
      url = GetAuthorizeUrl(wallet->one_time_string, true);
      break;
    }
    default:
      NOTREACHED();
  }

  return url;
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

  if (to == type::WalletStatus::PENDING) {
    DCHECK(from);
    g_show_newly_verified_wallet = *from == type::WalletStatus::NOT_CONNECTED;
  }
}

bool ShouldShowNewlyVerifiedWallet() {
  return g_show_newly_verified_wallet;
}

}  // namespace uphold
}  // namespace ledger
