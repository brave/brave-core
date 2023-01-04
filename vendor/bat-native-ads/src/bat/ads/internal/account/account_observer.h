/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"

namespace ads {

class AdType;
class ConfirmationType;
struct TransactionInfo;
struct WalletInfo;

class AccountObserver : public base::CheckedObserver {
 public:
  // Invoked when the |wallet| has updated.
  virtual void OnWalletDidUpdate(const WalletInfo& wallet) {}

  // Invoked when the |wallet| has changed.
  virtual void OnWalletDidChange(const WalletInfo& wallet) {}

  // Invoked if the wallet is invalid.
  virtual void OnInvalidWallet() {}

  // Invoked after successfully processing a deposit for |transaction|.
  virtual void OnDidProcessDeposit(const TransactionInfo& transaction) {}

  // Invoked after failing to process a deposit for |creative_instance_id|,
  // |ad_type| and |confirmation_type|.
  virtual void OnFailedToProcessDeposit(
      const std::string& creative_instance_id,
      const AdType& ad_type,
      const ConfirmationType& confirmation_type) {}

  // Invoked when the statement of accounts have changed.
  virtual void OnStatementOfAccountsDidChange() {}
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_
