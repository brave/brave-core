/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account_observer_unittest_helper.h"

namespace brave_ads {

AccountObserverForTesting::AccountObserverForTesting() = default;

AccountObserverForTesting::~AccountObserverForTesting() = default;

void AccountObserverForTesting::Reset() {
  did_initialize_wallet_ = false;
  failed_to_initialize_wallet_ = false;
  did_process_deposit_ = false;
  transaction_.reset();
  failed_to_process_deposit_ = false;
  statement_of_accounts_did_change_ = false;
}

void AccountObserverForTesting::OnDidInitializeWallet(
    const WalletInfo& /*wallet*/) {
  did_initialize_wallet_ = true;
}

void AccountObserverForTesting::OnFailedToInitializeWallet() {
  failed_to_initialize_wallet_ = true;
}

void AccountObserverForTesting::OnDidProcessDeposit(
    const TransactionInfo& transaction) {
  did_process_deposit_ = true;
  transaction_ = transaction;
}

void AccountObserverForTesting::OnFailedToProcessDeposit(
    const std::string& /*creative_instance_id*/,
    const AdType& /*ad_type*/,
    const ConfirmationType& /*confirmation_type*/) {
  failed_to_process_deposit_ = true;
}

void AccountObserverForTesting::OnStatementOfAccountsDidChange() {
  statement_of_accounts_did_change_ = true;
}

}  // namespace brave_ads
