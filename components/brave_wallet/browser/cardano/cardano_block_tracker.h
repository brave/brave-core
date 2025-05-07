/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_BLOCK_TRACKER_H_

#include <map>
#include <optional>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"

namespace brave_wallet {

class CardanoWalletService;

namespace cardano_rpc {
struct Block;
}  // namespace cardano_rpc

class CardanoBlockTracker : public BlockTracker {
 public:
  explicit CardanoBlockTracker(CardanoWalletService& cardano_wallet_service);
  ~CardanoBlockTracker() override;
  CardanoBlockTracker(const CardanoBlockTracker&) = delete;
  CardanoBlockTracker operator=(const CardanoBlockTracker&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnLatestHeightUpdated(const std::string& chain_id,
                                       uint32_t latest_height) = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void Start(const std::string& chain_id, base::TimeDelta interval) override;
  void RequestLatestBlock(const std::string& chain_id);

 private:
  std::optional<uint32_t> GetLatestHeight(const std::string& chain_id) const;
  void OnGetLatestBlock(
      const std::string& chain_id,
      base::expected<cardano_rpc::Block, std::string> latest_height);

  // <chain_id, block_height>
  std::map<std::string, uint32_t> latest_height_map_;
  base::ObserverList<Observer> observers_;

  const raw_ref<CardanoWalletService> cardano_wallet_service_;

  base::WeakPtrFactory<CardanoBlockTracker> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_BLOCK_TRACKER_H_
