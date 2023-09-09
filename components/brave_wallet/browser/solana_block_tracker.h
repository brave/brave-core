/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_BLOCK_TRACKER_H_

#include <map>
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
    virtual void OnLatestBlockhashUpdated(const std::string& chain_id,
                                          const std::string& latest_blockhash,
                                          uint64_t last_valid_block_height) = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // If timer is already running, it will be replaced with new interval
  void Start(const std::string& chain_id, base::TimeDelta interval) override;

  using GetLatestBlockhashCallback =
      base::OnceCallback<void(const std::string& latest_blockhash,
                              uint64_t last_valid_block_height,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetLatestBlockhash(const std::string& chain_id,
                          GetLatestBlockhashCallback callback,
                          bool try_cached_value);

 private:
  void OnGetLatestBlockhash(const std::string& chain_id,
                            GetLatestBlockhashCallback callback,
                            const std::string& latest_blockhash,
                            uint64_t last_valid_block_height,
                            mojom::SolanaProviderError error,
                            const std::string& error_message);
  // <chain_id, lastest_blockhash>
  std::map<std::string, std::string> latest_blockhash_map_;
  // <chain_id, last_valid_block_height>
  std::map<std::string, uint64_t> last_valid_block_height_map_;
  // <chain_id, latest_blockhash_expired_time>
  std::map<std::string, base::Time> latest_blockhash_expired_time_map_;
  base::ObserverList<Observer> observers_;

  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  base::WeakPtrFactory<SolanaBlockTracker> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_BLOCK_TRACKER_H_
