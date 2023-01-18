// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/vpn_dns_handler.h"

#include <string>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_dns_delegate.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/vpn_utils.h"
#include "brave/components/brave_vpn/browser/connection/win/utils.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"

namespace {
// Repeating interval to check the connection is live.
constexpr int kCheckConnectionIntervalInSeconds = 3;
}  // namespace

namespace brave_vpn {

VpnDnsHandler::VpnDnsHandler(BraveVpnDnsDelegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate_);
}
VpnDnsHandler::~VpnDnsHandler() {
  CloseWatchers();
}

bool VpnDnsHandler::SetupPlatformFilters(HANDLE engine_handle,
                                         const std::string& name) {
  if (platform_filters_result_for_testing_.has_value())
    return platform_filters_result_for_testing_.value();

  return AddWpmFilters(engine_handle, name);
}

void VpnDnsHandler::SetPlatformFiltersResultForTesting(bool value) {
  platform_filters_result_for_testing_ = value;
}
void VpnDnsHandler::SetCloseEngineResultForTesting(bool value) {
  close_engine_result_for_testing_ = value;
}

void VpnDnsHandler::SetConnectionResultForTesting(
    internal::CheckConnectionResult result) {
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

internal::CheckConnectionResult VpnDnsHandler::GetVpnEntryStatus() {
  VLOG(1) << __func__;
  if (connection_result_for_testing_.has_value()) {
    return connection_result_for_testing_.value();
  }
  return internal::CheckConnection(
      base::UTF8ToWide(brave_vpn::kBraveVPNEntryName));
}

void VpnDnsHandler::UpdateFiltersState() {
  VLOG(1) << __func__;
  switch (GetVpnEntryStatus()) {
    case internal::CheckConnectionResult::CONNECTED:
      VLOG(1) << "BraveVPN connected, set filters";
      if (IsActive()) {
        VLOG(1) << "Filters are already installed";
        return;
      }
      if (!SetFilters(base::UTF8ToWide(brave_vpn::kBraveVPNEntryName))) {
        VLOG(1) << "Failed to set DNS filters";
        Exit();
        return;
      }
      break;
    case internal::CheckConnectionResult::DISCONNECTED:
      VLOG(1) << "BraveVPN Disconnected, remove filters";
      if (!RemoveFilters(base::UTF8ToWide(brave_vpn::kBraveVPNEntryName))) {
        VLOG(1) << "Failed to remove DNS filters";
        Exit();
        break;
      }
      // Reset service launch counter if dns filters successfully removed.
      ResetLaunchCounter();
      Exit();
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

void VpnDnsHandler::Exit() {
  CloseWatchers();
  delegate_->SignalExit();
}

void VpnDnsHandler::OnObjectSignaled(HANDLE object) {
  VLOG(1) << __func__;
  // We receive events from all connections in the system and filter here
  // only expected brave vpn event.
  if (object != event_handle_for_vpn_)
    return;
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

}  // namespace brave_vpn
