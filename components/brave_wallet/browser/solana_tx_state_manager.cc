/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_state_manager.h"

#include "base/notreached.h"

namespace brave_wallet {

SolanaTxStateManager::SolanaTxStateManager(PrefService* prefs,
                                           JsonRpcService* json_rpc_service)
    : TxStateManager(prefs, json_rpc_service) {}

SolanaTxStateManager::~SolanaTxStateManager() = default;

std::unique_ptr<TxMeta> SolanaTxStateManager::ValueToTxMeta(
    const base::Value& value) {
  NOTIMPLEMENTED();
  return nullptr;
}

std::string SolanaTxStateManager::GetTxPrefPathPrefix() {
  NOTIMPLEMENTED();
  return "";
}

}  // namespace brave_wallet
