/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NONCE_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NONCE_TRACKER_H_

#include <string>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

class JsonRpcService;
class TxStateManager;
class TxMeta;

class NonceTracker {
 public:
  NonceTracker(TxStateManager* tx_state_manager,
               JsonRpcService* json_rpc_service);
  virtual ~NonceTracker();
  NonceTracker(const NonceTracker&) = delete;

  using GetNextNonceCallback =
      base::OnceCallback<void(bool success, uint256_t nonce)>;

  virtual void GetNextNonce(const std::string& from,
                            GetNextNonceCallback callback) = 0;
  virtual std::string GetChecksumAddress(const std::string& address) = 0;
  virtual uint256_t GetHighestContinuousFrom(
      const std::vector<std::unique_ptr<TxMeta>>& metas,
      uint256_t start) = 0;
  virtual uint256_t GetHighestLocallyConfirmed(
      const std::vector<std::unique_ptr<TxMeta>>& metas) = 0;

  base::Lock* GetLock() { return &nonce_lock_; }

  JsonRpcService* GetJsonRpcService();
  void OnGetNetworkNonce(const std::string& from,
                         GetNextNonceCallback callback,
                         uint256_t result,
                         mojom::ProviderError error,
                         const std::string& error_message);

 private:
  raw_ptr<TxStateManager> tx_state_manager_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  base::Lock nonce_lock_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_NONCE_TRACKER_H_
