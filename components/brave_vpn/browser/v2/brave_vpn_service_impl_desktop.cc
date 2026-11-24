/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/notimplemented.h"
#include "base/types/expected.h"
#include "brave/components/brave_vpn/browser/v2/api/brave_vpn_api_client.h"
#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"
#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "brave/components/version_info/version_info.h"
#include "components/version_info/version_info.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

namespace brave_vpn::v2 {
namespace {
std::string GetTimeZoneName() {
  std::unique_ptr<icu::TimeZone> zone(icu::TimeZone::createDefault());
  icu::UnicodeString id;
  zone->getID(id);
  std::string current_time_zone;
  id.toUTF8String<std::string>(current_time_zone);
  return current_time_zone;
}

// Bridges the endpoint client's typed result to the mojom
// (bool success, string response) contract used by CreateSupportTicket.
void RunCreateSupportTicketCallback(
    BraveVpnServiceImpl::CreateSupportTicketCallback callback,
    base::expected<std::string, std::string> result) {
  const bool success = result.has_value();
  std::move(callback).Run(success,
                          success ? std::string() : std::move(result).error());
}
}  // namespace

bool BraveVpnServiceImpl::IsConnected() const {
  return connection_state_ == mojom::ConnectionState::CONNECTED &&
         IsPurchased();
}

void BraveVpnServiceImpl::ToggleConnection() {
  if (connection_state_ == mojom::ConnectionState::CONNECTED ||
      connection_state_ == mojom::ConnectionState::CONNECTING) {
    Disconnect();
  } else {
    Connect();
  }
}

mojom::ConnectionState BraveVpnServiceImpl::GetConnectionState() const {
  return connection_state_;
}

std::string BraveVpnServiceImpl::GetLastConnectionError() const {
  return last_connection_error_;
}

void BraveVpnServiceImpl::RecordWidgetUsageMetrics(bool new_usage) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::GetConnectionState(
    GetConnectionStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(connection_state_);
}

void BraveVpnServiceImpl::Connect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* host = GetConnectedBrowserHost()) {
    host->Connect();
  } else {
    VLOG(1) << __func__ << ": agent not available";
  }
}

void BraveVpnServiceImpl::Disconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* host = GetConnectedBrowserHost()) {
    host->Disconnect();
  } else {
    VLOG(1) << __func__ << ": agent not available";
  }
}

void BraveVpnServiceImpl::GetSelectedRegion(
    GetSelectedRegionCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  if (agent_state_) {
    std::move(callback).Run(agent_state_->selected_region.Clone());
    return;
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  std::move(callback).Run(mojom::Region::New());
}

void BraveVpnServiceImpl::SetSelectedRegion(mojom::RegionPtr region) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* host = GetConnectedBrowserHost()) {
    host->SelectRegion(region->name);
  } else {
    VLOG(1) << __func__ << ": agent not available";
  }
}

void BraveVpnServiceImpl::ClearSelectedRegion() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* host = GetConnectedBrowserHost()) {
    host->SelectRegionAutomatically();
  } else {
    VLOG(1) << __func__ << ": agent not available";
  }
}

void BraveVpnServiceImpl::GetProductUrls(GetProductUrlsCallback callback) {
  std::move(callback).Run(mojom::ProductUrls::New(
      kFeedbackUrl, kAboutUrl, GetManageUrl(GetCurrentEnvironment())));
}

void BraveVpnServiceImpl::CreateSupportTicket(
    const std::string& email,
    const std::string& subject,
    const std::string& body,
    CreateSupportTicketCallback callback) {
  if (!api_client_) {
    std::move(callback).Run(false, {});
    return;
  }
  std::optional<std::string> subscriber_credential =
      purchased_state_manager_
          ? purchased_state_manager_->GetSubscriberCredential()
          : std::nullopt;

  api_client_->CreateSupportTicket(
      base::BindOnce(&RunCreateSupportTicketCallback, std::move(callback)),
      email, subject, body, subscriber_credential.value_or(""),
      GetTimeZoneName());
}

void BraveVpnServiceImpl::GetSupportData(GetSupportDataCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string last_hostname;
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  if (agent_state_) {
    last_hostname = agent_state_->last_hostname;
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  std::move(callback).Run(
      version_info::GetBraveVersionWithoutChromiumMajorVersion(),
      std::string(version_info::GetOSType()), std::move(last_hostname),
      GetTimeZoneName());
}

void BraveVpnServiceImpl::ResetConnectionState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* host = GetConnectedBrowserHost()) {
    host->ResetConnectionState();
  } else {
    VLOG(1) << __func__ << ": agent not available";
  }
}

void BraveVpnServiceImpl::EnableAutoReconnect(bool enable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* host = GetConnectedBrowserHost()) {
    host->SetAutoReconnectState(enable);
  } else {
    VLOG(1) << __func__ << ": agent not available";
  }
}

void BraveVpnServiceImpl::GetAutoReconnectState(
    GetAutoReconnectStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool auto_reconnect_available = false;
  bool auto_reconnect_enabled = false;
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  if (agent_state_) {
    auto_reconnect_available = true;
    auto_reconnect_enabled = agent_state_->auto_reconnect_enabled;
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  std::move(callback).Run(auto_reconnect_available, auto_reconnect_enabled);
}

void BraveVpnServiceImpl::EnableSmartProxyRouting(bool enable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* host = GetConnectedBrowserHost()) {
    host->SetSmartProxyRoutingState(enable);
  } else {
    VLOG(1) << __func__ << ": agent not available";
  }
}

void BraveVpnServiceImpl::GetSmartProxyRoutingState(
    GetSmartProxyRoutingStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool smart_proxy_routing_enabled = false;
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  if (agent_state_) {
    smart_proxy_routing_enabled = agent_state_->smart_proxy_routing_enabled;
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  std::move(callback).Run(smart_proxy_routing_enabled);
}

void BraveVpnServiceImpl::AllowLanTraffic(bool allow) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* host = GetConnectedBrowserHost()) {
    host->SetAllowLanTrafficState(allow);
  } else {
    VLOG(1) << __func__ << ": agent not available";
  }
}

void BraveVpnServiceImpl::GetAllowLanTraffic(
    GetAllowLanTrafficCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool allow_lan_traffic_available = false;
  bool allow_lan_traffic_enabled = false;
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  if (agent_state_) {
    allow_lan_traffic_available = agent_state_->allow_lan_traffic_available;
    allow_lan_traffic_enabled = agent_state_->allow_lan_traffic_enabled;
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  std::move(callback).Run(allow_lan_traffic_available,
                          allow_lan_traffic_enabled);
}

void BraveVpnServiceImpl::ReconnectToAgent() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  if (!agent_client_) {
    return;
  }
  VLOG(1) << "Retrying agent connection on user request";
  agent_client_->Reset();
  agent_client_->EnsureConnected();
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
}

mojom::BrowserHost* BraveVpnServiceImpl::GetConnectedBrowserHost() {
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  // The getters treat |agent_state_| as an indication that the agent is
  // connected, so the setters should also check the state to avoid inconsistent
  // behavior when the agent is not fully connected.
  if (agent_state_ && agent_client_ && agent_client_->browser_host()) {
    return agent_client_->browser_host();
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  return nullptr;
}

void BraveVpnServiceImpl::UpdateConnectionState(
    mojom::ConnectionState state,
    std::optional<std::string> connection_error) {
  const bool connection_error_changed =
      connection_error.has_value() &&
      *connection_error != last_connection_error_;
  if (connection_state_ == state && !connection_error_changed) {
    return;
  }
  connection_state_ = state;
  if (connection_error) {
    last_connection_error_ = std::move(*connection_error);
  }
  NotifyConnectionStateChanged(state);
}

void BraveVpnServiceImpl::SetConnectionStateForTesting(  // IN-TEST
    mojom::ConnectionState state) {
  UpdateConnectionState(state);
}

void BraveVpnServiceImpl::SetPurchasedStateForTesting(  // IN-TEST
    const std::string& env,
    mojom::PurchasedState state) {
  purchased_state_manager_->SetPurchasedState(env, state);
}

}  // namespace brave_vpn::v2
