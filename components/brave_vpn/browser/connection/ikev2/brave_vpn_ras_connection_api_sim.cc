/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/ikev2/brave_vpn_ras_connection_api_sim.h"

#include <memory>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
#include "components/version_info/channel.h"

namespace brave_vpn {

BraveVPNOSConnectionAPISim::BraveVPNOSConnectionAPISim(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs)
    : BraveVPNOSConnectionAPIBase(url_loader_factory,
                                  local_prefs,
                                  version_info::Channel::DEFAULT) {}

BraveVPNOSConnectionAPISim::~BraveVPNOSConnectionAPISim() = default;

void BraveVPNOSConnectionAPISim::CreateVPNConnectionImpl(
    const BraveVPNConnectionInfo& info) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveVPNOSConnectionAPISim::OnCreated,
                     weak_factory_.GetWeakPtr(), info.connection_name(), true));
}

void BraveVPNOSConnectionAPISim::ConnectImpl(const std::string& name) {
  disconnect_requested_ = false;

  // Determine connection success randomly.
  const bool success = base::RandInt(0, 9) > 3;
  // Simulate connection success
  if (success) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnIsConnecting,
                                  weak_factory_.GetWeakPtr(), name));

    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&BraveVPNOSConnectionAPISim::OnConnected,
                       weak_factory_.GetWeakPtr(), name, true),
        base::Seconds(1));
    return;
  }

  // Simulate connection failure
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnIsConnecting,
                                weak_factory_.GetWeakPtr(), name));
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveVPNOSConnectionAPISim::OnConnected,
                     weak_factory_.GetWeakPtr(), name, false),
      base::Seconds(1));
}

void BraveVPNOSConnectionAPISim::DisconnectImpl(const std::string& name) {
  disconnect_requested_ = true;

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnIsDisconnecting,
                                weak_factory_.GetWeakPtr(), name));

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnDisconnected,
                                weak_factory_.GetWeakPtr(), name, true));
}

void BraveVPNOSConnectionAPISim::Connect() {
  BraveVPNOSConnectionAPIBase::Connect();
}

void BraveVPNOSConnectionAPISim::CheckConnectionImpl(const std::string& name) {
  check_connection_called_ = true;
}

void BraveVPNOSConnectionAPISim::OnCreated(const std::string& name,
                                           bool success) {
  if (!success) {
    return;
  }
  connection_created_ = true;
  BraveVPNOSConnectionAPIBase::OnCreated();
}

void BraveVPNOSConnectionAPISim::Disconnect() {
  BraveVPNOSConnectionAPIBase::Disconnect();
}

void BraveVPNOSConnectionAPISim::CheckConnection() {
  BraveVPNOSConnectionAPIBase::CheckConnection();
}

bool BraveVPNOSConnectionAPISim::IsConnectionCreated() const {
  return connection_created_;
}

bool BraveVPNOSConnectionAPISim::IsConnectionChecked() const {
  return check_connection_called_;
}

void BraveVPNOSConnectionAPISim::OnConnected(const std::string& name,
                                             bool success) {
  // Cancelling connecting request simulation.
  if (disconnect_requested_) {
    disconnect_requested_ = false;
    return;
  }

  success ? BraveVPNOSConnectionAPIBase::OnConnected()
          : BraveVPNOSConnectionAPIBase::OnConnectFailed();
}

void BraveVPNOSConnectionAPISim::OnIsConnecting(const std::string& name) {
  BraveVPNOSConnectionAPIBase::OnIsConnecting();
}

void BraveVPNOSConnectionAPISim::OnDisconnected(const std::string& name,
                                                bool success) {
  if (!success) {
    return;
  }

  BraveVPNOSConnectionAPIBase::OnDisconnected();
}

void BraveVPNOSConnectionAPISim::OnIsDisconnecting(const std::string& name) {
  BraveVPNOSConnectionAPIBase::OnIsDisconnecting();
}

void BraveVPNOSConnectionAPISim::SetNetworkAvailableForTesting(bool value) {
  network_available_ = value;
}

bool BraveVPNOSConnectionAPISim::IsPlatformNetworkAvailable() {
  if (network_available_.has_value()) {
    return network_available_.value();
  }
  return true;
}

}  // namespace brave_vpn
