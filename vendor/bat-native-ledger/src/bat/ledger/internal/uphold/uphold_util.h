/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace uphold {

namespace notifications {
const char kBATNotAllowedForUser[] = "uphold_bat_not_allowed_for_user";
const char kBlockedUser[] = "uphold_blocked_user";
const char kPendingUser[] = "uphold_pending_user";
const char kRestrictedUser[] = "uphold_restricted_user";
}  // namespace notifications

const char kUrlStaging[] = "https://wallet-sandbox.uphold.com";
const char kUrlProduction[] = "https://uphold.com";
const char kFeeAddressStaging[] = "1b2b466f-5c15-49bf-995e-c91777d3da93";
const char kFeeAddressProduction[] = "b01e8c55-5004-4761-9e4b-01ec13e25c92";
const char kACAddressStaging[] = "1b2b466f-5c15-49bf-995e-c91777d3da93";
const char kACAddressProduction[] = "b01e8c55-5004-4761-9e4b-01ec13e25c92";

std::string GetClientId();

std::string GetClientSecret();

std::string GetUrl();

std::string GetFeeAddress();

std::string GetACAddress();

std::string GetAuthorizeUrl(const std::string& state, const bool kyc_flow);

std::string GetAddUrl(const std::string& address);

std::string GetWithdrawUrl(const std::string& address);

std::string GetSecondStepVerify();

std::string GetAccountUrl();

type::ExternalWalletPtr GenerateLinks(type::ExternalWalletPtr wallet);

std::string GenerateVerifyLink(type::ExternalWalletPtr wallet);

void OnWalletStatusChange(LedgerImpl* ledger,
                          absl::optional<type::WalletStatus> from,
                          type::WalletStatus to);

bool ShouldShowNewlyVerifiedWallet();

}  // namespace uphold
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_UTIL_H_
