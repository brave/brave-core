/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_UNITTEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_UNITTEST_HELPER_H_

#include <string>

#include "brave/components/brave_ads/core/internal/account/account_observer.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

class AdType;
class ConfirmationType;
struct WalletInfo;

class AccountObserverForTesting final : public AccountObserver {
 public:
  AccountObserverForTesting();

  AccountObserverForTesting(const AccountObserverForTesting&) = delete;
  AccountObserverForTesting& operator=(const AccountObserverForTesting&) =
      delete;

  AccountObserverForTesting(AccountObserverForTesting&&) noexcept = delete;
  AccountObserverForTesting& operator=(AccountObserverForTesting&&) noexcept =
      delete;

  ~AccountObserverForTesting() override;

  bool did_initialize_wallet() const { return did_initialize_wallet_; }

  bool failed_to_initialize_wallet() const {
    return failed_to_initialize_wallet_;
  }

  bool did_process_deposit() const { return did_process_deposit_; }

  const absl::optional<TransactionInfo>& transaction() const {
    return transaction_;
  }

  bool failed_to_process_deposit() const { return failed_to_process_deposit_; }

  bool statement_of_accounts_did_change() const {
    return statement_of_accounts_did_change_;
  }

  void Reset();

 private:
  // AccountObserver:
  void OnDidInitializeWallet(const WalletInfo& /*wallet*/) override;
  void OnFailedToInitializeWallet() override;
  void OnDidProcessDeposit(const TransactionInfo& transaction) override;
  void OnFailedToProcessDeposit(
      const std::string& /*creative_instance_id*/,
      const AdType& /*ad_type*/,
      const ConfirmationType& /*confirmation_type*/) override;
  void OnStatementOfAccountsDidChange() override;

  bool did_initialize_wallet_ = false;
  bool failed_to_initialize_wallet_ = false;
  bool did_process_deposit_ = false;
  absl::optional<TransactionInfo> transaction_;
  bool failed_to_process_deposit_ = false;
  bool statement_of_accounts_did_change_ = false;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_UNITTEST_HELPER_H_
