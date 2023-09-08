/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_BLOCK_TRACKER_H_

#include <map>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

namespace bitcoin_rpc {
class BitcoinRpc;
}

class BitcoinBlockTracker : public BlockTracker {
 public:
  explicit BitcoinBlockTracker(bitcoin_rpc::BitcoinRpc* bitcoin_rpc);
  ~BitcoinBlockTracker() override;
  BitcoinBlockTracker(const BitcoinBlockTracker&) = delete;
  BitcoinBlockTracker operator=(const BitcoinBlockTracker&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnLatestHeightUpdated(const std::string& chain_id,
                                       uint32_t latest_height) = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void Start(const std::string& chain_id, base::TimeDelta interval) override;
  absl::optional<uint32_t> GetLatestHeight(const std::string& chain_id) const;

 private:
  void GetBlockHeight(const std::string& chain_id);
  void OnGetBlockHeight(const std::string& chain_id,
                        base::expected<uint32_t, std::string> latest_height);

  // <chain_id, block_height>
  std::map<std::string, uint32_t> latest_height_map_;
  base::ObserverList<Observer> observers_;

  raw_ptr<bitcoin_rpc::BitcoinRpc> bitcoin_rpc_ = nullptr;

  base::WeakPtrFactory<BitcoinBlockTracker> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_BLOCK_TRACKER_H_
