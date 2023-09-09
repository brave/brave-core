/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_BLOCK_TRACKER_H_

#include <map>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class JsonRpcService;

class FilBlockTracker : public BlockTracker {
 public:
  explicit FilBlockTracker(JsonRpcService* json_rpc_service);
  ~FilBlockTracker() override;
  FilBlockTracker(const FilBlockTracker&) = delete;
  FilBlockTracker operator=(const FilBlockTracker&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnLatestHeightUpdated(const std::string& chain_id,
                                       uint64_t latest_height) = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  using GetFilBlockHeightCallback =
      base::OnceCallback<void(uint64_t latest_height,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void GetFilBlockHeight(const std::string& chain_id,
                         GetFilBlockHeightCallback callback);
  uint64_t GetLatestHeight(const std::string& chain_id) const;
  // If timer is already running, it will be replaced with new interval
  void Start(const std::string& chain_id, base::TimeDelta interval) override;

 private:
  void OnGetFilBlockHeight(const std::string& chain_id,
                           GetFilBlockHeightCallback callback,
                           uint64_t latest_height,
                           mojom::FilecoinProviderError error,
                           const std::string& error_message);

  // <chain_id, block_height>
  std::map<std::string, uint64_t> latest_height_map_;
  base::ObserverList<Observer> observers_;

  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  base::WeakPtrFactory<FilBlockTracker> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_BLOCK_TRACKER_H_
