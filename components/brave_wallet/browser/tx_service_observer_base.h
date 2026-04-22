/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_SERVICE_OBSERVER_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_SERVICE_OBSERVER_BASE_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_WALLET));
namespace brave_wallet {

class TxServiceObserverBase : public mojom::TxServiceObserver {
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override {}
  void OnTxServiceReset() override {}
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_SERVICE_OBSERVER_BASE_H_
