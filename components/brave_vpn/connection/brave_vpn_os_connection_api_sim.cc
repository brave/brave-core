/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/connection/brave_vpn_os_connection_api_sim.h"

#include <memory>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/connection/brave_vpn_os_connection_api.h"

namespace brave_vpn {

// static
std::unique_ptr<BraveVPNOSConnectionAPI>
BraveVPNOSConnectionAPI::GetInstanceForTest() {
  return std::make_unique<BraveVPNOSConnectionAPISim>();
}

BraveVPNOSConnectionAPISim::BraveVPNOSConnectionAPISim() = default;
BraveVPNOSConnectionAPISim::~BraveVPNOSConnectionAPISim() = default;

void BraveVPNOSConnectionAPISim::CreateVPNConnectionImpl(
    const BraveVPNConnectionInfo& info) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveVPNOSConnectionAPISim::OnCreated,
                     weak_factory_.GetWeakPtr(), info.connection_name(), true));
}

std::string BraveVPNOSConnectionAPISim::GetHostname() const {
  return std::string();
}

void BraveVPNOSConnectionAPISim::ConnectImpl(const std::string& name) {
  disconnect_requested_ = false;

  // Determine connection success randomly.
  const bool success = base::RandInt(0, 9) > 3;
  // Simulate connection success
  if (success) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnIsConnecting,
                                  weak_factory_.GetWeakPtr(), name));

    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&BraveVPNOSConnectionAPISim::OnConnected,
                       weak_factory_.GetWeakPtr(), name, true),
        base::Seconds(1));
    return;
  }

  // Simulate connection failure
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnIsConnecting,
                                weak_factory_.GetWeakPtr(), name));
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveVPNOSConnectionAPISim::OnConnected,
                     weak_factory_.GetWeakPtr(), name, false),
      base::Seconds(1));
}

void BraveVPNOSConnectionAPISim::DisconnectImpl(const std::string& name) {
  disconnect_requested_ = true;

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnIsDisconnecting,
                                weak_factory_.GetWeakPtr(), name));

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnDisconnected,
                                weak_factory_.GetWeakPtr(), name, true));
}

void BraveVPNOSConnectionAPISim::ResetConnectionInfo() {
  BraveVPNOSConnectionAPIBase::ResetConnectionInfo();
}

void BraveVPNOSConnectionAPISim::Connect() {
  BraveVPNOSConnectionAPIBase::Connect();
}

void BraveVPNOSConnectionAPISim::RemoveVPNConnectionImpl(
    const std::string& name) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnRemoved,
                                weak_factory_.GetWeakPtr(), name, true));
}

void BraveVPNOSConnectionAPISim::CheckConnectionImpl(const std::string& name) {
  check_connection_called_ = true;
}

void BraveVPNOSConnectionAPISim::OnCreated(const std::string& name,
                                           bool success) {
  if (!success)
    return;
  connection_created_ = true;
  BraveVPNOSConnectionAPIBase::OnCreated();
}

void BraveVPNOSConnectionAPISim::SetConnectionState(
    mojom::ConnectionState state) {
  BraveVPNOSConnectionAPIBase::SetConnectionState(state);
}

void BraveVPNOSConnectionAPISim::Disconnect() {
  BraveVPNOSConnectionAPIBase::Disconnect();
}

void BraveVPNOSConnectionAPISim::ToggleConnection() {
  BraveVPNOSConnectionAPIBase::ToggleConnection();
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

void BraveVPNOSConnectionAPISim::UpdateAndNotifyConnectionStateChange(
    mojom::ConnectionState state) {
  BraveVPNOSConnectionAPIBase::UpdateAndNotifyConnectionStateChange(state);
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
  if (!success)
    return;

  BraveVPNOSConnectionAPIBase::OnDisconnected();
}

void BraveVPNOSConnectionAPISim::OnIsDisconnecting(const std::string& name) {
  BraveVPNOSConnectionAPIBase::OnIsDisconnecting();
}

void BraveVPNOSConnectionAPISim::OnRemoved(const std::string& name,
                                           bool success) {}

}  // namespace brave_vpn
