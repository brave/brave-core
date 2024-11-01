/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"

#include <optional>

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"

namespace brave_wallet {

EthTxStateManager::EthTxStateManager(
    TxStorageDelegate& delegate,
    AccountResolverDelegate& account_resolver_delegate)
    : TxStateManager(delegate, account_resolver_delegate) {}

EthTxStateManager::~EthTxStateManager() = default;

std::unique_ptr<EthTxMeta> EthTxStateManager::GetEthTx(const std::string& id) {
  return std::unique_ptr<EthTxMeta>{
      static_cast<EthTxMeta*>(TxStateManager::GetTx(id).release())};
}

std::unique_ptr<EthTxMeta> EthTxStateManager::ValueToEthTxMeta(
    const base::Value::Dict& value) {
  return std::unique_ptr<EthTxMeta>{
      static_cast<EthTxMeta*>(ValueToTxMeta(value).release())};
}

mojom::CoinType EthTxStateManager::GetCoinType() const {
  return mojom::CoinType::ETH;
}

std::unique_ptr<TxMeta> EthTxStateManager::ValueToTxMeta(
    const base::Value::Dict& value) {
  std::unique_ptr<EthTxMeta> meta = std::make_unique<EthTxMeta>();

  if (!ValueToBaseTxMeta(value, meta.get())) {
    return nullptr;
  }

  const base::Value::Dict* tx_receipt = value.FindDict("tx_receipt");
  if (!tx_receipt) {
    return nullptr;
  }
  std::optional<TransactionReceipt> tx_receipt_from_value =
      ValueToTransactionReceipt(*tx_receipt);
  if (!tx_receipt_from_value) {
    return nullptr;
  }
  meta->set_tx_receipt(*tx_receipt_from_value);

  const base::Value::Dict* tx = value.FindDict("tx");
  if (!tx) {
    return nullptr;
  }

  std::optional<bool> sign_only = value.FindBool("sign_only");
  if (sign_only) {
    meta->set_sign_only(*sign_only);
  }

  std::optional<int> type = tx->FindInt("type");
  if (!type) {
    return nullptr;
  }

  switch (static_cast<uint8_t>(*type)) {
    case 0: {
      std::optional<EthTransaction> tx_from_value =
          EthTransaction::FromValue(*tx);
      if (!tx_from_value) {
        return nullptr;
      }
      meta->set_tx(std::make_unique<EthTransaction>(*tx_from_value));
      break;
    }
    case 1: {
      std::optional<Eip2930Transaction> tx_from_value =
          Eip2930Transaction::FromValue(*tx);
      if (!tx_from_value) {
        return nullptr;
      }
      meta->set_tx(std::make_unique<Eip2930Transaction>(*tx_from_value));
      break;
    }
    case 2: {
      std::optional<Eip1559Transaction> tx_from_value =
          Eip1559Transaction::FromValue(*tx);
      if (!tx_from_value) {
        return nullptr;
      }
      meta->set_tx(std::make_unique<Eip1559Transaction>(*tx_from_value));
      break;
    }
    default:
      LOG(ERROR) << "tx type is not supported";
      break;
  }

  return meta;
}

}  // namespace brave_wallet
