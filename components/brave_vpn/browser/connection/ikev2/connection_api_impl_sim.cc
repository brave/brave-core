/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/ikev2/connection_api_impl_sim.h"

#include <memory>

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_vpn {

ConnectionAPIImplSim::ConnectionAPIImplSim(
    BraveVPNConnectionManager* manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : SystemVPNConnectionAPIImplBase(manager, url_loader_factory) {}

ConnectionAPIImplSim::~ConnectionAPIImplSim() = default;

void ConnectionAPIImplSim::CreateVPNConnectionImpl(
    const BraveVPNConnectionInfo& info) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&ConnectionAPIImplSim::OnCreated,
                     weak_factory_.GetWeakPtr(), info.connection_name(), true));
}

void ConnectionAPIImplSim::ConnectImpl(const std::string& name) {
  disconnect_requested_ = false;

  // Determine connection success randomly.
  const bool success = base::RandInt(0, 9) > 3;
  // Simulate connection success
  if (success) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&ConnectionAPIImplSim::OnIsConnecting,
                                  weak_factory_.GetWeakPtr(), name));

    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&ConnectionAPIImplSim::OnConnected,
                       weak_factory_.GetWeakPtr(), name, true),
        base::Seconds(1));
    return;
  }

  // Simulate connection failure
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ConnectionAPIImplSim::OnIsConnecting,
                                weak_factory_.GetWeakPtr(), name));
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ConnectionAPIImplSim::OnConnected,
                     weak_factory_.GetWeakPtr(), name, false),
      base::Seconds(1));
}

void ConnectionAPIImplSim::DisconnectImpl(const std::string& name) {
  disconnect_requested_ = true;

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ConnectionAPIImplSim::OnIsDisconnecting,
                                weak_factory_.GetWeakPtr(), name));

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ConnectionAPIImplSim::OnDisconnected,
                                weak_factory_.GetWeakPtr(), name, true));
}

void ConnectionAPIImplSim::Connect() {
  SystemVPNConnectionAPIImplBase::Connect();
}

void ConnectionAPIImplSim::CheckConnectionImpl(const std::string& name) {
  check_connection_called_ = true;
}

void ConnectionAPIImplSim::OnCreated(const std::string& name,
                                           bool success) {
  if (!success) {
    return;
  }
  connection_created_ = true;
  SystemVPNConnectionAPIImplBase::OnCreated();
}

void ConnectionAPIImplSim::Disconnect() {
  SystemVPNConnectionAPIImplBase::Disconnect();
}

void ConnectionAPIImplSim::CheckConnection() {
  SystemVPNConnectionAPIImplBase::CheckConnection();
}

bool ConnectionAPIImplSim::IsConnectionCreated() const {
  return connection_created_;
}

bool ConnectionAPIImplSim::IsConnectionChecked() const {
  return check_connection_called_;
}

void ConnectionAPIImplSim::OnConnected(const std::string& name,
                                             bool success) {
  // Cancelling connecting request simulation.
  if (disconnect_requested_) {
    disconnect_requested_ = false;
    return;
  }

  success ? SystemVPNConnectionAPIImplBase::OnConnected()
          : SystemVPNConnectionAPIImplBase::OnConnectFailed();
}

void ConnectionAPIImplSim::OnIsConnecting(const std::string& name) {
  SystemVPNConnectionAPIImplBase::OnIsConnecting();
}

void ConnectionAPIImplSim::OnDisconnected(const std::string& name,
                                                bool success) {
  if (!success) {
    return;
  }

  SystemVPNConnectionAPIImplBase::OnDisconnected();
}

void ConnectionAPIImplSim::OnIsDisconnecting(const std::string& name) {
  SystemVPNConnectionAPIImplBase::OnIsDisconnecting();
}

void ConnectionAPIImplSim::SetNetworkAvailableForTesting(bool value) {
  network_available_ = value;
}

bool ConnectionAPIImplSim::IsPlatformNetworkAvailable() {
  if (network_available_.has_value()) {
    return network_available_.value();
  }
  return true;
}

ConnectionAPIImpl::Type ConnectionAPIImplSim::type() const {
  return Type::IKEV2;
}

}  // namespace brave_vpn
