/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_MANAGER_H_

#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

class PrefService;

namespace brave_wallet {

class TxService;
class JsonRpcService;
class KeyringService;

class TxManager {
 public:
  TxManager(TxService* tx_service,
            JsonRpcService* json_rpc_service,
            KeyringService* keyring_service,
            PrefService* prefs);
  virtual ~TxManager() = default;

  using AddUnapprovedTransactionCallback =
      mojom::TxService::AddUnapprovedTransactionCallback;
  using ApproveTransactionCallback =
      mojom::TxService::ApproveTransactionCallback;
  using RejectTransactionCallback = mojom::TxService::RejectTransactionCallback;
  using GetAllTransactionInfoCallback =
      mojom::TxService::GetAllTransactionInfoCallback;
  using SpeedupOrCancelTransactionCallback =
      mojom::TxService::SpeedupOrCancelTransactionCallback;
  using RetryTransactionCallback = mojom::TxService::RetryTransactionCallback;
  using GetTransactionMessageToSignCallback =
      mojom::TxService::GetTransactionMessageToSignCallback;

  virtual void AddUnapprovedTransaction(mojom::TxDataUnionPtr tx_data_union,
                                        const std::string& from,
                                        AddUnapprovedTransactionCallback) = 0;
  virtual void ApproveTransaction(const std::string& tx_meta_id,
                                  ApproveTransactionCallback) = 0;
  virtual void RejectTransaction(const std::string& tx_meta_id,
                                 RejectTransactionCallback) = 0;
  virtual void GetAllTransactionInfo(const std::string& from,
                                     GetAllTransactionInfoCallback) = 0;

  virtual void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) = 0;
  virtual void RetryTransaction(const std::string& tx_meta_id,
                                RetryTransactionCallback callback) = 0;

  virtual void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) = 0;

  virtual void Reset() = 0;

 protected:
  raw_ptr<TxService> tx_service_ = nullptr;             // NOT OWNED
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;  // NOT OWNED
  raw_ptr<KeyringService> keyring_service_ = nullptr;   // NOT OWNED
  raw_ptr<PrefService> prefs_ = nullptr;                // NOT OWNED
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_MANAGER_H_
