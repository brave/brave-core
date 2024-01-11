// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_vpn/win/brave_vpn_helper/vpn_dns_handler.h"

#include <string>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_dns_delegate.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/vpn_utils.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/win/ras_utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_commands.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"

namespace brave_vpn {

namespace {
// If the user clicks connect vpn immediately after disconnecting, the service
// may not start sometimes because there is no notification about vpn connection
// from the system. We only get 2 'Network State Change...' events and there is
// no trigger event in the system log about the vpn connection state change.
// In order to manage this we keep service running for sometime and check if any
// subsequent events occur.
constexpr int kWaitingIntervalBeforeExitSec = 10;
}  // namespace

VpnDnsHandler::VpnDnsHandler(BraveVpnDnsDelegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate_);
}

VpnDnsHandler::~VpnDnsHandler() {
  CloseWatchers();
}

bool VpnDnsHandler::SetupPlatformFilters(HANDLE engine_handle,
                                         const std::string& name) {
  if (platform_filters_result_for_testing_.has_value()) {
    return platform_filters_result_for_testing_.value();
  }

  return AddWpmFilters(engine_handle, name);
}

void VpnDnsHandler::SetPlatformFiltersResultForTesting(bool value) {
  platform_filters_result_for_testing_ = value;
}
void VpnDnsHandler::SetCloseEngineResultForTesting(bool value) {
  close_engine_result_for_testing_ = value;
}

void VpnDnsHandler::SetConnectionResultForTesting(
    ras::CheckConnectionResult result) {
  connection_result_for_testing_ = result;
}

bool VpnDnsHandler::CloseEngineSession() {
  if (close_engine_result_for_testing_.has_value()) {
    return close_engine_result_for_testing_.value();
  }

  return CloseWpmSession(engine_);
}

bool VpnDnsHandler::SetFilters(const std::wstring& connection_name) {
  VLOG(1) << __func__ << ":" << connection_name;
  if (IsActive()) {
    VLOG(1) << "Filters activated for:" << connection_name;
    return true;
  }

  engine_ = OpenWpmSession();
  if (!engine_) {
    VLOG(1) << "Failed to open engine session";
    return false;
  }

  if (!SetupPlatformFilters(engine_, base::WideToASCII(connection_name))) {
    if (!RemoveFilters(connection_name)) {
      VLOG(1) << "Failed to remove DNS filters";
    }
    return false;
  }

  // Show system notification about connected vpn.
  brave_vpn::RunWireGuardCommandForUsers(
      brave_vpn::kBraveVpnWireguardServiceNotifyConnectedSwitchName);

  return true;
}

bool VpnDnsHandler::IsActive() const {
  return engine_ != nullptr;
}

bool VpnDnsHandler::RemoveFilters(const std::wstring& connection_name) {
  VLOG(1) << __func__ << ":" << connection_name;
  if (!IsActive()) {
    VLOG(1) << "No active filters";
    return true;
  }
  bool success = CloseEngineSession();
  if (success) {
    engine_ = nullptr;
  }
  return success;
}

ras::CheckConnectionResult VpnDnsHandler::GetVpnEntryStatus() {
  VLOG(1) << __func__;
  if (connection_result_for_testing_.has_value()) {
    return connection_result_for_testing_.value();
  }
  return ras::CheckConnection(GetBraveVPNConnectionName());
}

void VpnDnsHandler::DisconnectVPN() {
  if (connection_result_for_testing_.has_value()) {
    connection_result_for_testing_ = ras::CheckConnectionResult::DISCONNECTED;
    return;
  }

  auto result = ras::DisconnectEntry(GetBraveVPNConnectionName());
  if (!result.success) {
    VLOG(1) << "Failed to disconnect entry:" << result.error_description;
  }
}

void VpnDnsHandler::UpdateFiltersState() {
  VLOG(1) << __func__;
  switch (GetVpnEntryStatus()) {
    case ras::CheckConnectionResult::CONNECTED:
      VLOG(1) << "BraveVPN connected, set filters";
      if (IsActive()) {
        VLOG(1) << "Filters are already installed";
        return;
      }
      if (!SetFilters(GetBraveVPNConnectionName())) {
        VLOG(1) << "Failed to set DNS filters";
        DisconnectVPN();
        ScheduleExit();
        return;
      }
      SetFiltersInstalledFlag();
      break;
    case ras::CheckConnectionResult::DISCONNECTED:
      VLOG(1) << "BraveVPN Disconnected, remove filters";
      if (!RemoveFilters(GetBraveVPNConnectionName())) {
        VLOG(1) << "Failed to remove DNS filters";
        Exit();
        break;
      }
      // Reset service launch counter if dns filters successfully removed.
      ResetFiltersInstalledFlag();
      ScheduleExit();
      break;
    default:
      VLOG(1) << "BraveVPN is connecting, try later after "
              << kCheckConnectionIntervalInSeconds << " seconds.";
      break;
  }
}

void VpnDnsHandler::CloseWatchers() {
  if (event_handle_for_vpn_) {
    CloseHandle(event_handle_for_vpn_);
    event_handle_for_vpn_ = nullptr;
  }
  periodic_timer_.Stop();
}

int VpnDnsHandler::GetWaitingIntervalBeforeExit() {
  if (waiting_interval_before_exit_for_testing_.has_value()) {
    return waiting_interval_before_exit_for_testing_.value();
  }
  return kWaitingIntervalBeforeExitSec;
}

void VpnDnsHandler::SetWaitingIntervalBeforeExitForTesting(int value) {
  waiting_interval_before_exit_for_testing_ = value;
}

void VpnDnsHandler::ScheduleExit() {
  if (exit_timer_.IsRunning()) {
    return;
  }
  exit_timer_.Start(
      FROM_HERE, base::Seconds(GetWaitingIntervalBeforeExit()),
      base::BindOnce(&VpnDnsHandler::Exit, weak_factory_.GetWeakPtr()));
}

void VpnDnsHandler::Exit() {
  if (GetVpnEntryStatus() == ras::CheckConnectionResult::CONNECTED) {
    VLOG(1) << __func__ << " vpn is active, do not exit";
    return;
  }
  CloseWatchers();
  delegate_->SignalExit();
}

void VpnDnsHandler::OnObjectSignaled(HANDLE object) {
  VLOG(1) << __func__;
  // We receive events from all connections in the system and filter here
  // only expected brave vpn event.
  if (object != event_handle_for_vpn_) {
    return;
  }
  if (exit_timer_.IsRunning()) {
    exit_timer_.Stop();
  }
  UpdateFiltersState();
}

void VpnDnsHandler::SubscribeForRasNotifications(HANDLE event_handle) {
  VLOG(1) << __func__;
  if (!SubscribeRasConnectionNotification(event_handle)) {
    VLOG(1) << "Failed to subscripbe for vpn notifications";
  }
}

void VpnDnsHandler::StartVPNConnectionChangeMonitoring() {
  DCHECK(!event_handle_for_vpn_);
  DCHECK(!IsActive());

  event_handle_for_vpn_ = CreateEvent(NULL, false, false, NULL);
  SubscribeForRasNotifications(event_handle_for_vpn_);

  connected_disconnected_event_watcher_.StartWatchingMultipleTimes(
      event_handle_for_vpn_, this);

  periodic_timer_.Start(FROM_HERE,
                        base::Seconds(kCheckConnectionIntervalInSeconds),
                        base::BindRepeating(&VpnDnsHandler::UpdateFiltersState,
                                            weak_factory_.GetWeakPtr()));
  UpdateFiltersState();
}

bool VpnDnsHandler::IsExitTimerRunningForTesting() {
  return exit_timer_.IsRunning();
}

}  // namespace brave_vpn
