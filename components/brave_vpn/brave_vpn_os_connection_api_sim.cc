/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_os_connection_api_sim.h"

#include "base/logging.h"
#include "base/notreached.h"
#include "base/threading/sequenced_task_runner_handle.h"

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
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnConnected,
                                weak_factory_.GetWeakPtr(), name, true));
}

void BraveVPNOSConnectionAPISim::Disconnect(const std::string& name) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnDisconnected,
                                weak_factory_.GetWeakPtr(), name, true));
}

void BraveVPNOSConnectionAPISim::RemoveVPNConnection(const std::string& name) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPISim::OnRemoved,
                                weak_factory_.GetWeakPtr(), name, true));
}

void BraveVPNOSConnectionAPISim::OnCreated(const std::string& name,
                                           bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnCreated(name);
}

void BraveVPNOSConnectionAPISim::OnConnected(const std::string& name,
                                             bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnConnected(name);
}

void BraveVPNOSConnectionAPISim::OnDisconnected(const std::string& name,
                                                bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnDisconnected(name);
}

void BraveVPNOSConnectionAPISim::OnRemoved(const std::string& name,
                                           bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnRemoved(name);
}

}  // namespace brave_vpn
