/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct TransactionInfo;
struct WalletInfo;

class AccountObserver : public base::CheckedObserver {
 public:
  // Invoked when the `wallet` did initialize.
  virtual void OnDidInitializeWallet(const WalletInfo& wallet) {}

  // Invoked if the wallet is invalid.
  virtual void OnFailedToInitializeWallet() {}

  // Invoked after successfully processing a deposit for `transaction`.
  virtual void OnDidProcessDeposit(const TransactionInfo& transaction) {}

  // Invoked after failing to process a deposit for `creative_instance_id`,
  // `mojom_ad_type` and `mojom_confirmation_type`.
  virtual void OnFailedToProcessDeposit(
      const std::string& creative_instance_id,
      mojom::AdType mojom_ad_type,
      mojom::ConfirmationType mojom_confirmation_type) {}
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_
