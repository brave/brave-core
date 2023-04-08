/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_BLOCK_TRACKER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"

namespace brave_wallet {

class JsonRpcService;
class BitcoinWalletService;

class BitcoinBlockTracker : public BlockTracker {
 public:
  BitcoinBlockTracker(const std::string& network_id,
                      JsonRpcService* json_rpc_service,
                      BitcoinWalletService* bitcoin_wallet_service);
  ~BitcoinBlockTracker() override;
  BitcoinBlockTracker(const BitcoinBlockTracker&) = delete;
  BitcoinBlockTracker operator=(const BitcoinBlockTracker&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnLatestHeightUpdated(const std::string& network_id,
                                       uint64_t latest_height) = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  uint64_t latest_height() const { return latest_height_; }
  void Start(base::TimeDelta interval) override;

 private:
  void GetBlockHeight();
  void OnGetBlockHeight(base::expected<uint32_t, std::string> latest_height);

  uint64_t latest_height_ = 0;
  base::ObserverList<Observer> observers_;

  std::string network_id_;
  raw_ptr<BitcoinWalletService> bitcoin_wallet_service_ = nullptr;

  base::WeakPtrFactory<BitcoinBlockTracker> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_BLOCK_TRACKER_H_
