/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_BLOCK_TRACKER_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_header.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"

namespace brave_wallet {

class PolkadotSubstrateRpc;

class PolkadotBlockTracker : public BlockTracker {
 public:
  explicit PolkadotBlockTracker(PolkadotSubstrateRpc& polkadot_rpc);
  ~PolkadotBlockTracker() override;

  PolkadotBlockTracker(const PolkadotBlockTracker&) = delete;
  PolkadotBlockTracker& operator=(const PolkadotBlockTracker&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    // Fires for each latest block check.
    virtual void OnLatestBlock(const std::string& chain_id,
                               uint32_t block_num) = 0;
  };

  // BlockTracker
  void Start(const std::string& chain_id, base::TimeDelta interval) override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  void GetLatestBlock(const std::string& chain_id);
  void OnGetFinalizedHead(
      const std::string& chain_id,
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> block_hash,
      std::optional<std::string> err_str);
  void OnGetFinalizedBlockHeader(
      const std::string& chain_id,
      std::optional<PolkadotBlockHeader> block_header,
      std::optional<std::string> err_str);

  base::flat_map<std::string, std::array<uint8_t, kPolkadotBlockHashSize>>
      latest_block_hashes_map_;
  base::ObserverList<Observer> observers_;
  raw_ref<PolkadotSubstrateRpc> polkadot_rpc_;
  base::WeakPtrFactory<PolkadotBlockTracker> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_BLOCK_TRACKER_H_
