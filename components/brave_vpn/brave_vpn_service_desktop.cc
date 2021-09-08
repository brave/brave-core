/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_vpn/switches.h"

namespace {

constexpr char kBraveVPNEntryName[] = "BraveVPN";

bool GetVPNCredentialsFromSwitch(brave_vpn::BraveVPNConnectionInfo* info) {
  DCHECK(info);
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(brave_vpn::switches::kBraveVPNTestCredentials))
    return false;

  std::string value =
      cmd->GetSwitchValueASCII(brave_vpn::switches::kBraveVPNTestCredentials);
  std::vector<std::string> tokens = base::SplitString(
      value, ":", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (tokens.size() == 4) {
    info->SetConnectionInfo(tokens[0], tokens[1], tokens[2], tokens[3]);
    return true;
  }

  LOG(ERROR) << __func__ << ": Invalid credentials";
  return false;
}

brave_vpn::BraveVPNOSConnectionAPI* GetBraveVPNConnectionAPI() {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(brave_vpn::switches::kBraveVPNSimulation))
    return brave_vpn::BraveVPNOSConnectionAPI::GetInstanceForTest();
  return brave_vpn::BraveVPNOSConnectionAPI::GetInstance();
}

}  // namespace

BraveVpnServiceDesktop::BraveVpnServiceDesktop(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : BraveVpnService(url_loader_factory) {
  observed_.Observe(GetBraveVPNConnectionAPI());

  GetBraveVPNConnectionAPI()->set_target_vpn_entry_name(kBraveVPNEntryName);
  GetBraveVPNConnectionAPI()->CheckConnection(kBraveVPNEntryName);

  CheckPurchasedStatus();
}

BraveVpnServiceDesktop::~BraveVpnServiceDesktop() = default;

void BraveVpnServiceDesktop::Shutdown() {
  BraveVpnService::Shutdown();

  observed_.Reset();
  observers_.Clear();
}

void BraveVpnServiceDesktop::OnCreated(const std::string& name) {
  for (Observer& obs : observers_)
    obs.OnConnectionCreated();

  for (const auto& obs : mojo_observers_)
    obs->OnConnectionCreated();
}

void BraveVpnServiceDesktop::OnRemoved(const std::string& name) {
  for (Observer& obs : observers_)
    obs.OnConnectionRemoved();

  for (const auto& obs : mojo_observers_)
    obs->OnConnectionRemoved();
}

void BraveVpnServiceDesktop::OnConnected(const std::string& name) {
  if (state_ == ConnectionState::CONNECTED)
    return;

  state_ = ConnectionState::CONNECTED;

  for (Observer& obs : observers_)
    obs.OnConnectionStateChanged(ConnectionState::CONNECTED);

  for (const auto& obs : mojo_observers_)
    obs->OnConnectionStateChanged(ConnectionState::CONNECTED);
}

void BraveVpnServiceDesktop::OnIsConnecting(const std::string& name) {
  if (state_ == ConnectionState::CONNECTING)
    return;

  state_ = ConnectionState::CONNECTING;

  for (Observer& obs : observers_)
    obs.OnConnectionStateChanged(ConnectionState::CONNECTING);

  for (const auto& obs : mojo_observers_)
    obs->OnConnectionStateChanged(ConnectionState::CONNECTING);
}

void BraveVpnServiceDesktop::OnConnectFailed(const std::string& name) {
  if (state_ == ConnectionState::CONNECT_FAILED)
    return;

  state_ = ConnectionState::CONNECT_FAILED;

  for (Observer& obs : observers_)
    obs.OnConnectionStateChanged(ConnectionState::CONNECT_FAILED);

  for (const auto& obs : mojo_observers_)
    obs->OnConnectionStateChanged(ConnectionState::CONNECT_FAILED);
}

void BraveVpnServiceDesktop::OnDisconnected(const std::string& name) {
  if (state_ == ConnectionState::DISCONNECTED)
    return;

  state_ = ConnectionState::DISCONNECTED;

  for (Observer& obs : observers_)
    obs.OnConnectionStateChanged(ConnectionState::DISCONNECTED);

  for (const auto& obs : mojo_observers_)
    obs->OnConnectionStateChanged(ConnectionState::DISCONNECTED);
}

void BraveVpnServiceDesktop::OnIsDisconnecting(const std::string& name) {
  if (state_ == ConnectionState::DISCONNECTING)
    return;

  state_ = ConnectionState::DISCONNECTING;

  for (Observer& obs : observers_)
    obs.OnConnectionStateChanged(ConnectionState::DISCONNECTING);

  for (const auto& obs : mojo_observers_)
    obs->OnConnectionStateChanged(ConnectionState::DISCONNECTING);
}

void BraveVpnServiceDesktop::CreateVPNConnection() {
  GetBraveVPNConnectionAPI()->CreateVPNConnection(GetConnectionInfo());
}

void BraveVpnServiceDesktop::RemoveVPNConnnection() {
  GetBraveVPNConnectionAPI()->RemoveVPNConnection(
      GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::Connect() {
  if (state_ == ConnectionState::CONNECTING)
    return;

  GetBraveVPNConnectionAPI()->Connect(GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::Disconnect() {
  if (state_ == ConnectionState::DISCONNECTING)
    return;

  GetBraveVPNConnectionAPI()->Disconnect(GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::CheckPurchasedStatus() {
  // TODO(simonhong): Should notify to observers when purchased status is
  // changed.
  brave_vpn::BraveVPNConnectionInfo info;
  if (GetVPNCredentialsFromSwitch(&info)) {
    is_purchased_user_ = true;
    CreateVPNConnection();
    return;
  }

  NOTIMPLEMENTED();
}

void BraveVpnServiceDesktop::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVpnServiceDesktop::AddObserver(
    mojo::PendingRemote<brave_vpn::mojom::ServiceObserver> observer) {
  mojo_observers_.Add(std::move(observer));
}

void BraveVpnServiceDesktop::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

brave_vpn::BraveVPNConnectionInfo BraveVpnServiceDesktop::GetConnectionInfo() {
  brave_vpn::BraveVPNConnectionInfo info;
  if (GetVPNCredentialsFromSwitch(&info))
    return info;

  // TODO(simonhong): Get real credentials from payment service.
  NOTIMPLEMENTED();
  return info;
}

void BraveVpnServiceDesktop::BindInterface(
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BraveVpnServiceDesktop::GetConnectionState(
    GetConnectionStateCallback callback) {
  std::move(callback).Run(state_);
}
