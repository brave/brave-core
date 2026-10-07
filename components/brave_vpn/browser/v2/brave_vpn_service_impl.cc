/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/notimplemented.h"
#include "base/sequence_checker.h"
#include "base/types/to_address.h"
#include "brave/components/brave_vpn/browser/v2/api/brave_vpn_api_client.h"
#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"
#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "build/build_config.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

// The agent apps build only on desktop, so the related code assumes desktop
// only APIs are available; asserted once rather than guarded at call sites.
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
static_assert(!BUILDFLAG(IS_ANDROID));
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)

namespace brave_vpn::v2 {

BraveVpnServiceImpl::BraveVpnServiceImpl(
    PrefService* local_prefs,
    PrefService* profile_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    GetSkusServiceCallback skus_service_getter)
    : profile_prefs_(CHECK_DEREF(profile_prefs)),
      api_client_(
          std::make_unique<BraveVpnApiClient>(std::move(url_loader_factory))),
      skus_client_(
          std::make_unique<SkusServiceClient>(std::move(skus_service_getter))) {
  DCHECK(IsBraveVPNFeatureEnabled());
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  agent_client_ = std::make_unique<AgentClient>();
  agent_client_->AddObserver(this);
  agent_launcher_ = std::make_unique<AgentLauncher>();
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  purchased_state_manager_ = std::make_unique<PurchasedStateManager>(
      local_prefs, api_client_.get(), skus_client_.get(),
      base::BindRepeating(&BraveVpnServiceImpl::OnPurchasedStateChanged,
                          base::Unretained(this)));
}

BraveVpnServiceImpl::~BraveVpnServiceImpl() = default;

bool BraveVpnServiceImpl::IsBraveVPNEnabled() const {
  return ::brave_vpn::IsBraveVPNEnabled(base::to_address(profile_prefs_));
}

bool BraveVpnServiceImpl::IsPurchased() const {
  if (!purchased_state_manager_) {
    return false;
  }
  return purchased_state_manager_->IsPurchased();
}

void BraveVpnServiceImpl::ReloadPurchasedState() {
  if (purchased_state_manager_) {
    purchased_state_manager_->Reload();
  }
}

std::string BraveVpnServiceImpl::GetCurrentEnvironment() const {
  if (!purchased_state_manager_) {
    return {};
  }
  return purchased_state_manager_->GetCurrentEnvironment();
}

void BraveVpnServiceImpl::GetPurchasedState(
    GetPurchasedStateCallback callback) {
  if (purchased_state_manager_) {
    std::move(callback).Run(purchased_state_manager_->GetInfo().Clone());
    return;
  }
  std::move(callback).Run(mojom::PurchasedInfo::New(
      mojom::PurchasedState::NOT_PURCHASED, std::nullopt));
}

void BraveVpnServiceImpl::LoadPurchasedState(const std::string& domain) {
  if (purchased_state_manager_) {
    purchased_state_manager_->Load(domain);
  }
}

void BraveVpnServiceImpl::GetAllRegions(GetAllRegionsCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run({});
}

void BraveVpnServiceImpl::Shutdown() {
  weak_factory_.InvalidateWeakPtrs();
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  agent_launcher_.reset();
  agent_client_.reset();
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  purchased_state_manager_.reset();
  api_client_.reset();
  skus_client_->Reset();
  BraveVpnService::Shutdown();
}

void BraveVpnServiceImpl::OnPurchasedStateChanged(
    mojom::PurchasedState state,
    std::optional<std::string> description) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  UpdateAgentConnection(state);
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)

  NotifyPurchasedStateChanged(state, description);

  // TODO: If purchased state changed to PURCHASED on desktop, we can attempt to
  // install VPN helper, etc. We also need to make sure the agent, once
  // connected, fetches the region data - BEFORE we actually send a notification
  // to the UI.
}

#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)

void BraveVpnServiceImpl::UpdateAgentConnection(mojom::PurchasedState state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!agent_client_) {
    return;
  }

  switch (state) {
    case mojom::PurchasedState::PURCHASED:
      // The only state that justifies waking the agent. Policy still gets
      // checked: a profile where the VPN is disabled must not open a connection
      // at all.
      if (IsBraveVPNEnabled()) {
        agent_client_->EnsureConnected();
      }
      return;
    case mojom::PurchasedState::NOT_PURCHASED:
      // Nothing left to drive, so stop holding a session the agent would keep
      // alive for this profile. Reset() also clears any refusal verdict, so a
      // later purchase starts from a clean attempt.
      agent_client_->Reset();
      // Clear leftover connection state if any.
      UpdateConnectionState(mojom::ConnectionState::DISCONNECTED,
                            std::string());
      return;
    case mojom::PurchasedState::LOADING:
    case mojom::PurchasedState::SESSION_EXPIRED:
    case mojom::PurchasedState::FAILED:
    case mojom::PurchasedState::OUT_OF_CREDENTIALS:
      // Deliberately no change. All four are recoverable and can coexist with a
      // live tunnel, so an existing connection stays up - the person still has
      // to be able to disconnect - while none of them is a reason to open one.
      return;
  }
}

void BraveVpnServiceImpl::OnAgentConnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << "Agent session established";
}

void BraveVpnServiceImpl::OnAgentSessionStable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << "Agent session is now stable";

  // Clears any leftover connection error.
  // TODO(https://github.com/brave/brave-browser/issues/59013)
  // Replace with a new agent-specific connection state.
  if (connection_state_ == mojom::ConnectionState::CONNECT_NOT_ALLOWED) {
    UpdateConnectionState(mojom::ConnectionState::DISCONNECTED, std::string());
  }
}

void BraveVpnServiceImpl::OnAgentDisconnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << "Agent session lost";

  // Agent disconnection on its own is not a verdict: the client reconnects, and
  // an unrecoverable run of failures arrives in another ("connection failed")
  // notification. But make sure the "connected" VPN state is not reported to
  // the user if there's no connection to the agent, and hence there is no
  // explicit knowledge that the VPN tunnel is up.
  if (connection_state_ == mojom::ConnectionState::CONNECTED) {
    UpdateConnectionState(mojom::ConnectionState::DISCONNECTED, std::string());
  }
}

void BraveVpnServiceImpl::OnAgentNotRunning() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LOG(ERROR) << "Agent is not running";

  // The agent process is apparently not running. While the agent client will
  // retry on its own, launch the agent process. This is a workaround for the
  // case where the agent is not started by the privileged helper.
  if (agent_launcher_) {
    agent_launcher_->Launch(base::BindOnce(
        &BraveVpnServiceImpl::OnAgentLaunchFailed, weak_factory_.GetWeakPtr()));
  }
}

void BraveVpnServiceImpl::OnAgentConnectionFailed(AgentClient::Error error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string connection_error_message{AgentClient::ErrorToString(error)};
  LOG(ERROR) << "Agent connection failed: " << connection_error_message;

  // TODO(https://github.com/brave/brave-browser/issues/59013)
  // Replace with a new agent-specific connection state.
  UpdateConnectionState(mojom::ConnectionState::CONNECT_NOT_ALLOWED,
                        std::move(connection_error_message));
}

void BraveVpnServiceImpl::OnAgentLaunchFailed(
    AgentLauncher::LaunchError error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string connection_error_message{AgentLauncher::ErrorToString(error)};
  LOG(ERROR) << "Failed to launch agent: " << connection_error_message;

  // Ignore launch errors if the service has already shut down.
  if (!agent_client_) {
    return;
  }

  // Ignore launch errors if the agent is there: started by a helper, by a
  // sibling profile, manually by user. This call becomes a report about a
  // launch that has been overtaken by external events.
  if (agent_client_->is_connected()) {
    VLOG(1) << "Ignoring launch failure; the agent is already connected";
    return;
  }

  // Purchased state is untouched - the subscription is valid and the UI stays
  // mostly enabled though in an error state. Just stop the retry loop; the next
  // user-initiated connect starts a fresh sequence.
  agent_client_->Reset();

  // TODO(https://github.com/brave/brave-browser/issues/59013)
  // Replace with a new agent-specific connection state.
  UpdateConnectionState(mojom::ConnectionState::CONNECT_NOT_ALLOWED,
                        std::move(connection_error_message));
}

#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)

}  // namespace brave_vpn::v2
