/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_

#include "base/observer_list.h"

namespace ads {

struct CatalogIssuersInfo;
struct WalletInfo;

class AccountObserver : public base::CheckedObserver {
 public:
  // Invoked when the wallet has updated
  virtual void OnWalletDidUpdate(const WalletInfo& wallet) {}

  // Invoked when a wallet has changed
  virtual void OnWalletDidChange(const WalletInfo& wallet) {}

  // Invoked if the wallet is invalid
  virtual void OnInvalidWallet() {}

  // Invoked when the catalog issuers have changed
  virtual void OnCatalogIssuersDidChange(
      const CatalogIssuersInfo& catalog_issuers) {}

  // Invoked when the statement of accounts has changed
  virtual void OnStatementOfAccountsDidChange() {}

 protected:
  ~AccountObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_OBSERVER_H_
