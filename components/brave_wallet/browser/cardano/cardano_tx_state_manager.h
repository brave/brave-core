/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TX_STATE_MANAGER_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/tx_state_manager.h"

namespace brave_wallet {

class TxMeta;
class CardanoTxMeta;
class TxStorageDelegate;

class CardanoTxStateManager : public TxStateManager {
 public:
  CardanoTxStateManager(TxStorageDelegate& delegate,
                        AccountResolverDelegate& account_resolver_delegate);
  ~CardanoTxStateManager() override;
  CardanoTxStateManager(const CardanoTxStateManager&) = delete;
  CardanoTxStateManager operator=(const CardanoTxStateManager&) = delete;

  std::unique_ptr<CardanoTxMeta> GetCardanoTx(const std::string& id);
  std::unique_ptr<CardanoTxMeta> ValueToCardanoTxMeta(
      const base::DictValue& value);

 private:
  mojom::CoinType GetCoinType() const override;

  std::unique_ptr<TxMeta> ValueToTxMeta(const base::DictValue& value) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TX_STATE_MANAGER_H_
