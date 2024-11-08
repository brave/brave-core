/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_LOGS_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_LOGS_TRACKER_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

class JsonRpcService;

class EthLogsTracker {
 public:
  explicit EthLogsTracker(JsonRpcService* json_rpc_service);
  EthLogsTracker(const EthLogsTracker&) = delete;
  EthLogsTracker& operator=(const EthLogsTracker&) = delete;
  EthLogsTracker(const EthLogsTracker&&) = delete;
  EthLogsTracker& operator=(const EthLogsTracker&&) = delete;

  ~EthLogsTracker();

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnLogsReceived(const std::string& subscription,
                                base::Value rawlogs) = 0;
  };

  // If timer is already running, it will be replaced with new interval
  void Start(const std::string& chain_id, base::TimeDelta interval);
  void Stop();
  bool IsRunning() const;

  void AddSubscriber(const std::string& subscription_id,
                     base::Value::Dict filter);
  void RemoveSubscriber(const std::string& subscription_id);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  void GetLogs(const std::string& chain_id);
  void OnGetLogs(const std::string& subscription,
                 const std::vector<Log>& logs,
                 base::Value rawlogs,
                 mojom::ProviderError error,
                 const std::string& error_message);

  base::RepeatingTimer timer_;
  raw_ptr<JsonRpcService, DanglingUntriaged> json_rpc_service_ = nullptr;

  std::map<std::string, base::Value::Dict> eth_logs_subscription_info_;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<EthLogsTracker> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_LOGS_TRACKER_H_
