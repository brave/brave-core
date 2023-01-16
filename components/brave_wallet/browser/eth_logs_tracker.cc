/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_logs_tracker.h"

#include <string>
#include <vector>

namespace brave_wallet {

EthLogsTracker::EthLogsTracker(JsonRpcService* json_rpc_service)
    : json_rpc_service_(json_rpc_service) {
  DCHECK(json_rpc_service_);
}

EthLogsTracker::~EthLogsTracker() = default;

void EthLogsTracker::Start(base::TimeDelta interval) {
  timer_.Start(FROM_HERE, interval,
               base::BindRepeating(&EthLogsTracker::GetLogs,
                                   weak_factory_.GetWeakPtr()));
}

void EthLogsTracker::Stop() {
  timer_.Stop();
}

bool EthLogsTracker::IsRunning() const {
  return timer_.IsRunning();
}

void EthLogsTracker::AddObserver(EthLogsTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void EthLogsTracker::RemoveObserver(EthLogsTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void EthLogsTracker::GetLogs() {
  const auto chain_id = json_rpc_service_->GetChainId(mojom::CoinType::ETH);

  json_rpc_service_->EthGetLogs(
      chain_id, {}, {}, {}, {},
      base::BindOnce(&EthLogsTracker::OnGetLogs, weak_factory_.GetWeakPtr()));
}

void EthLogsTracker::OnGetLogs([[maybe_unused]] const std::vector<Log>& logs,
                               base::Value rawlogs,
                               mojom::ProviderError error,
                               const std::string& error_message) {
  if (error == mojom::ProviderError::kSuccess && rawlogs.is_dict()) {
    for (auto& observer : observers_)
      observer.OnLogsReceived(rawlogs.Clone());
  } else {
    LOG(ERROR) << "OnGetLogs failed";
  }
}

}  // namespace brave_wallet
