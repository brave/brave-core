/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_manager.h"

#include "base/check.h"

namespace brave_wallet {

TxManager::TxManager(TxService* tx_service,
                     JsonRpcService* json_rpc_service,
                     KeyringService* keyring_service,
                     PrefService* prefs)
    : tx_service_(tx_service),
      json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      prefs_(prefs) {
  DCHECK(tx_service_);
  DCHECK(json_rpc_service_);
  DCHECK(keyring_service_);
}

}  // namespace brave_wallet
