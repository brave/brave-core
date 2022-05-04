/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_BLOCK_TRACKER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class JsonRpcService;

class SolanaBlockTracker : public BlockTracker {
 public:
  explicit SolanaBlockTracker(JsonRpcService* json_rpc_service);
  ~SolanaBlockTracker() override;
  SolanaBlockTracker(const SolanaBlockTracker&) = delete;
  SolanaBlockTracker operator=(const SolanaBlockTracker&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnLatestBlockhashUpdated(const std::string& latest_blockhash,
                                          uint64_t last_valid_block_height) = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // If timer is already running, it will be replaced with new interval
  void Start(base::TimeDelta interval) override;
  void Stop() override;

  std::string latest_blockhash() const { return latest_blockhash_; }
  uint64_t last_valid_block_height() const { return last_valid_block_height_; }

  using GetLatestBlockhashCallback =
      base::OnceCallback<void(const std::string& latest_blockhash,
                              uint64_t last_valid_block_height,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetLatestBlockhash(GetLatestBlockhashCallback callback,
                          bool try_cached_value);

 private:
  void OnGetLatestBlockhash(GetLatestBlockhashCallback callback,
                            const std::string& latest_blockhash,
                            uint64_t last_valid_block_height,
                            mojom::SolanaProviderError error,
                            const std::string& error_message);

  std::string latest_blockhash_;
  uint64_t last_valid_block_height_ = 0;
  base::Time latest_blockhash_expired_time_;
  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<SolanaBlockTracker> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_BLOCK_TRACKER_H_
