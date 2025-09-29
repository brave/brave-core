/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_BLOCK_TRACKER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class PolkadotBlockTracker : public BlockTracker {
 public:
  PolkadotBlockTracker();
  ~PolkadotBlockTracker() override;

  PolkadotBlockTracker(const PolkadotBlockTracker&) = delete;
  PolkadotBlockTracker& operator=(const PolkadotBlockTracker&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    // Fires for each latest block check
    virtual void OnLatestBlock(const std::string& chain_id,
                               uint64_t block_num) = 0;
    // Only fires when there is a new block
    virtual void OnNewBlock(const std::string& chain_id,
                            uint64_t block_num) = 0;
  };

  // BlockTracker
  void Start(const std::string& chain_id, base::TimeDelta interval) override;
  void Stop(const std::string& chain_id) override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  void GetLatestBlock(const std::string& chain_id);
  void OnGetLatestBlock(const std::string& chain_id,
                        uint64_t block_num,
                        mojom::ProviderError error,
                        const std::string& error_message);

  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<PolkadotBlockTracker> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_BLOCK_TRACKER_H_
