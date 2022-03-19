/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_os_connection_api_sim.h"

#include "base/logging.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"

namespace brave_vpn {

// static
BraveVPNOSConnectionAPI* BraveVPNOSConnectionAPI::GetInstanceForTest() {
  static base::NoDestructor<BraveVPNOSConnectionAPISim> s_manager;
  return s_manager.get();
}

BraveVPNOSConnectionAPISim::BraveVPNOSConnectionAPISim() = default;
BraveVPNOSConnectionAPISim::~BraveVPNOSConnectionAPISim() = default;

void BraveVPNOSConnectionAPISim::CreateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveVPNOSConnectionAPISim::OnCreated,
                     weak_factory_.GetWeakPtr(), info.connection_name(), true));
}

void BraveVPNOSConnectionAPISim::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NOTIMPLEMENTED();
}

void BraveVPNOSConnectionAPISim::Connect(const std::string& name) {
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

void BraveVPNOSConnectionAPISim::Disconnect(const std::string& name) {
  disconnect_requested_ = true;

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnIsDisconnecting,
                                weak_factory_.GetWeakPtr(), name));

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnDisconnected,
                                weak_factory_.GetWeakPtr(), name, true));
}

void BraveVPNOSConnectionAPISim::RemoveVPNConnection(const std::string& name) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnRemoved,
                                weak_factory_.GetWeakPtr(), name, true));
}

void BraveVPNOSConnectionAPISim::CheckConnection(const std::string& name) {
  // Do nothing.
}

void BraveVPNOSConnectionAPISim::OnCreated(const std::string& name,
                                           bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnCreated();
}

void BraveVPNOSConnectionAPISim::OnConnected(const std::string& name,
                                             bool success) {
  // Cancelling connecting request simulation.
  if (disconnect_requested_) {
    disconnect_requested_ = false;
    return;
  }

  for (Observer& obs : observers_)
    success ? obs.OnConnected() : obs.OnConnectFailed();
}

void BraveVPNOSConnectionAPISim::OnIsConnecting(const std::string& name) {
  for (Observer& obs : observers_)
    obs.OnIsConnecting();
}

void BraveVPNOSConnectionAPISim::OnDisconnected(const std::string& name,
                                                bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnDisconnected();
}

void BraveVPNOSConnectionAPISim::OnIsDisconnecting(const std::string& name) {
  for (Observer& obs : observers_)
    obs.OnIsDisconnecting();
}

void BraveVPNOSConnectionAPISim::OnRemoved(const std::string& name,
                                           bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnRemoved();
}

}  // namespace brave_vpn
