/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_BLOCK_TRACKER_H_

#include <string>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

class JsonRpcService;

class EthBlockTracker {
 public:
  explicit EthBlockTracker(JsonRpcService* json_rpc_service);
  ~EthBlockTracker();
  EthBlockTracker(const EthBlockTracker&) = delete;
  EthBlockTracker operator=(const EthBlockTracker&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    // Fires for each latest block check
    virtual void OnLatestBlock(uint256_t block_num) = 0;
    // Only fires when there is a new block
    virtual void OnNewBlock(uint256_t block_num) = 0;
  };

  // If timer is already running, it will be replaced with new interval
  void Start(base::TimeDelta interval);
  void Stop();
  bool IsRunning() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  uint256_t GetCurrentBlock() const { return current_block_; }

  void CheckForLatestBlock(
      base::OnceCallback<void(uint256_t block_num,
                              mojom::ProviderError error,
                              const std::string& error_message)>);

 private:
  void SendGetBlockNumber(
      base::OnceCallback<void(uint256_t block_num,
                              mojom::ProviderError error,
                              const std::string& error_message)>);
  void GetBlockNumber();
  void OnGetBlockNumber(uint256_t block_num,
                        mojom::ProviderError error,
                        const std::string& error_message);

  uint256_t current_block_ = 0;
  base::RepeatingTimer timer_;

  base::ObserverList<Observer> observers_;

  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;

  base::WeakPtrFactory<EthBlockTracker> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_BLOCK_TRACKER_H_
