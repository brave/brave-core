/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_
#define BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_

#include "base/observer_list.h"

namespace ads {

struct CatalogIssuersInfo;
struct WalletInfo;

class AccountObserver : public base::CheckedObserver {
 public:
  // Invoked when the wallet has changed
  virtual void OnWalletChanged(
      const WalletInfo& wallet) {}

  // Invoked when a wallet is restored
  virtual void OnWalletRestored(
      const WalletInfo& wallet) {}

  // Invoked if the wallet is invalid
  virtual void OnWalletInvalid() {}

  // Invoked when the catalog issuers have changed
  virtual void OnCatalogIssuersChanged(
      const CatalogIssuersInfo& catalog_issuers) {}

  // Invoked when ad rewards have changed
  virtual void OnAdRewardsChanged() {}

  // Invoked when transactions have changed
  virtual void OnTransactionsChanged() {}

  // Invoked when uncleared transactions have been processed
  virtual void OnUnclearedTransactionsProcessed() {}

 protected:
  ~AccountObserver() override = default;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_
