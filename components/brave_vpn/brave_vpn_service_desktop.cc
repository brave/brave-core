/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"

#include <vector>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_vpn/switches.h"

namespace {

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
}

void BraveVpnServiceDesktop::OnRemoved(const std::string& name) {
  for (Observer& obs : observers_)
    obs.OnConnectionRemoved();
}

void BraveVpnServiceDesktop::OnConnected(const std::string& name) {
  is_connected_ = true;

  for (Observer& obs : observers_)
    obs.OnConnectionStateChanged(true);
}

void BraveVpnServiceDesktop::OnDisconnected(const std::string& name) {
  is_connected_ = false;

  for (Observer& obs : observers_)
    obs.OnConnectionStateChanged(false);
}

void BraveVpnServiceDesktop::CreateVPNConnection() {
  GetBraveVPNConnectionAPI()->CreateVPNConnection(GetConnectionInfo());
}

void BraveVpnServiceDesktop::RemoveVPNConnnection() {
  GetBraveVPNConnectionAPI()->RemoveVPNConnection(
      GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::Connect() {
  GetBraveVPNConnectionAPI()->Connect(GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::Disconnect() {
  GetBraveVPNConnectionAPI()->Disconnect(GetConnectionInfo().connection_name());
}

bool BraveVpnServiceDesktop::IsConnected() const {
  return is_connected_;
}

void BraveVpnServiceDesktop::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVpnServiceDesktop::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

brave_vpn::BraveVPNConnectionInfo BraveVpnServiceDesktop::GetConnectionInfo() {
  brave_vpn::BraveVPNConnectionInfo info;
  if (!GetVPNCredentialsFromSwitch(&info)) {
    // TODO(simonhong): Get real credentials from payment service.
    NOTIMPLEMENTED();
  }
  return info;
}
