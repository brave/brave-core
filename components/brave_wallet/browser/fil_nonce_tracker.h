/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_NONCE_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_NONCE_TRACKER_H_

#include <string>

#include "brave/components/brave_wallet/browser/nonce_tracker.h"

namespace brave_wallet {

class JsonRpcService;
class TxStateManager;

class FilNonceTracker : public NonceTracker {
 public:
  FilNonceTracker(TxStateManager* tx_state_manager,
                  JsonRpcService* json_rpc_service);
  ~FilNonceTracker() override;
  FilNonceTracker operator=(const FilNonceTracker&) = delete;

  // NonceTracker
  void GetNextNonce(const std::string& from,
                    GetNextNonceCallback callback) override;
  std::string GetChecksumAddress(const std::string& address) override;
  uint256_t GetHighestLocallyConfirmed(
      const std::vector<std::unique_ptr<TxMeta>>& metas) override;
  uint256_t GetHighestContinuousFrom(
      const std::vector<std::unique_ptr<TxMeta>>& metas,
      uint256_t start) override;

 private:
  base::WeakPtrFactory<FilNonceTracker> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_NONCE_TRACKER_H_
