/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"

#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/common/fil_address.h"

namespace brave_wallet {

FilTxStateManager::FilTxStateManager(PrefService* prefs,
                                     JsonRpcService* json_rpc_service)
    : TxStateManager(prefs, json_rpc_service), weak_factory_(this) {}

FilTxStateManager::~FilTxStateManager() = default;

std::unique_ptr<FilTxMeta> FilTxStateManager::GetFilTx(const std::string& id) {
  return std::unique_ptr<FilTxMeta>{
      static_cast<FilTxMeta*>(TxStateManager::GetTx(id).release())};
}

std::string FilTxStateManager::GetTxPrefPathPrefix() {
  return "fil." + json_rpc_service_->GetChainId(mojom::CoinType::FIL);
}

std::unique_ptr<TxMeta> FilTxStateManager::ValueToTxMeta(
    const base::Value& value) {
  std::unique_ptr<FilTxMeta> meta = std::make_unique<FilTxMeta>();

  if (!TxStateManager::ValueToTxMeta(value, meta.get()))
    return nullptr;
  const base::Value* tx = value.FindKey("tx");
  if (!tx)
    return nullptr;
  absl::optional<FilTransaction> tx_from_value = FilTransaction::FromValue(*tx);
  if (!tx_from_value)
    return nullptr;
  meta->set_tx(std::make_unique<FilTransaction>(*tx_from_value));
  return meta;
}

}  // namespace brave_wallet
