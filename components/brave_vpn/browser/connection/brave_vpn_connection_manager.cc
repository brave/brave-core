/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/browser/connection/connection_api_impl.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_vpn {

BraveVPNConnectionManager::BraveVPNConnectionManager(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    base::RepeatingCallback<bool()> service_installer)
    : local_prefs_(local_prefs),
      url_loader_factory_(url_loader_factory),
      region_data_manager_(url_loader_factory, local_prefs),
      weak_factory_(this) {
  DCHECK(url_loader_factory_);
  install_system_service_callback_ = std::move(service_installer);

#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
  wireguard_enabled_.Init(
      prefs::kBraveVPNWireguardEnabled, local_prefs_,
      base::BindRepeating(&BraveVPNConnectionManager::UpdateConnectionAPIImpl,
                          weak_factory_.GetWeakPtr()));
#endif
  // Safe to use Unretained here because |region_data_manager_| is owned
  // instance.
  region_data_manager_.set_selected_region_changed_callback(base::BindRepeating(
      &BraveVPNConnectionManager::NotifySelectedRegionChanged,
      base::Unretained(this)));
  region_data_manager_.set_region_data_ready_callback(
      base::BindRepeating(&BraveVPNConnectionManager::NotifyRegionDataReady,
                          base::Unretained(this)));
}

BraveVPNConnectionManager::~BraveVPNConnectionManager() = default;

BraveVPNRegionDataManager& BraveVPNConnectionManager::GetRegionDataManager() {
  return region_data_manager_;
}

void BraveVPNConnectionManager::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVPNConnectionManager::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveVPNConnectionManager::NotifyRegionDataReady(bool ready) const {
  for (auto& obs : observers_) {
    obs.OnRegionDataReady(ready);
  }
}

void BraveVPNConnectionManager::NotifySelectedRegionChanged(
    const std::string& name) const {
  for (auto& obs : observers_) {
    obs.OnSelectedRegionChanged(name);
  }
}

void BraveVPNConnectionManager::UpdateConnectionAPIImpl() {
  if (!connection_api_impl_getter_) {
    CHECK_IS_TEST();
    return;
  }

  // This could be called multiple times, so don't reset current connection
  // if prefs is matched with current |connection_api_impl_|.
  const bool wireguard_enabled =
#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
      wireguard_enabled_.GetValue();
#else
      false;
#endif

  if (!connection_api_impl_) {
    // Create new connection api impl.
    VLOG(2) << __func__
            << " : Create new connection api impl based on current prefs - "
               "wireguard_enabled("
            << wireguard_enabled << ">";
    connection_api_impl_ = connection_api_impl_getter_.Run(
        this, url_loader_factory_, wireguard_enabled);
    return;
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
  if (wireguard_enabled &&
      connection_api_impl_->type() == ConnectionAPIImpl::Type::WIREGUARD) {
    VLOG(2) << __func__ << " : Already have wireguard connection api impl.";
    return;
  }

  if (!wireguard_enabled &&
      connection_api_impl_->type() == ConnectionAPIImpl::Type::IKEV2) {
    VLOG(2) << __func__ << " : Already have ikev2 connection api impl.";
    return;
  }
#endif

  VLOG(2) << __func__
          << " : Create new connection api impl based on current prefs - "
             "wireguard_enabled("
          << wireguard_enabled << ">";
  connection_api_impl_ = connection_api_impl_getter_.Run(
      this, url_loader_factory_, wireguard_enabled);
}

mojom::ConnectionState BraveVPNConnectionManager::GetConnectionState() const {
  if (connection_api_impl_) {
    return connection_api_impl_->GetConnectionState();
  }

  return mojom::ConnectionState::DISCONNECTED;
}

void BraveVPNConnectionManager::ResetConnectionState() {
  if (connection_api_impl_) {
    connection_api_impl_->ResetConnectionState();
  }
}

void BraveVPNConnectionManager::Connect() {
  if (ScheduleConnectRequestIfNeeded()) {
    return;
  }

  if (connection_api_impl_) {
    connection_api_impl_->Connect();
  }
}

void BraveVPNConnectionManager::Disconnect() {
  if (connection_api_impl_) {
    connection_api_impl_->Disconnect();
  }
}

void BraveVPNConnectionManager::CheckConnection() {
  if (connection_api_impl_) {
    connection_api_impl_->CheckConnection();
  }
}

void BraveVPNConnectionManager::SetSelectedRegion(const std::string& name) {
  // TODO(simonhong): This method could be implemented here instead of impl
  // class.
  if (connection_api_impl_) {
    connection_api_impl_->SetSelectedRegion(name);
  }
}

std::string BraveVPNConnectionManager::GetHostname() const {
  if (connection_api_impl_) {
    return connection_api_impl_->GetHostname();
  }

  return {};
}

std::string BraveVPNConnectionManager::GetLastConnectionError() const {
  if (connection_api_impl_) {
    return connection_api_impl_->GetLastConnectionError();
  }

  return {};
}

void BraveVPNConnectionManager::ToggleConnection() {
  if (connection_api_impl_) {
    connection_api_impl_->ToggleConnection();
  }
}

std::string BraveVPNConnectionManager::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void BraveVPNConnectionManager::MaybeInstallSystemServices() {
  if (!install_system_service_callback_) {
    VLOG(2) << __func__ << " : no install system service callback set";

    UpdateConnectionAPIImpl();
    return;
  }

  // Installation should only be called once per session.
  // It's safe to call more than once because the install itself checks if
  // the services are already registered before doing anything.
  if (system_service_installed_event_.is_signaled()) {
    VLOG(2)
        << __func__
        << " : installation has already been performed this session; exiting";
    return;
  }

  // This API could be called more than once because BraveVpnService is a
  // per-profile service. If service install is in-progress now, just return.
  if (install_in_progress_) {
    VLOG(2) << __func__ << " : install already in progress; exiting";
    return;
  }

#if BUILDFLAG(IS_WIN)
  install_in_progress_ = true;
  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTaskAndReplyWithResult(
          FROM_HERE, install_system_service_callback_,
          base::BindOnce(
              &BraveVPNConnectionManager::OnInstallSystemServicesCompleted,
              weak_factory_.GetWeakPtr()));
#endif
}

void BraveVPNConnectionManager::OnInstallSystemServicesCompleted(bool success) {
  VLOG(1) << "OnInstallSystemServicesCompleted: success=" << success;
  if (success) {
#if BUILDFLAG(IS_WIN)
    // Update prefs first before signaling the event because the event could
    // check the prefs.
    EnableWireguardIfPossible(local_prefs_);
#endif
    system_service_installed_event_.Signal();
  } else {
    // On success, UpdateConnectionAPIImpl() will be called by prefs changing
    // notification.
    UpdateConnectionAPIImpl();
  }
  install_in_progress_ = false;
}

bool BraveVPNConnectionManager::ScheduleConnectRequestIfNeeded() {
  if (!install_in_progress_) {
    return false;
  }

  system_service_installed_event_.Post(
      FROM_HERE, base::BindOnce(&BraveVPNConnectionManager::Connect,
                                weak_factory_.GetWeakPtr()));
  return true;
}

void BraveVPNConnectionManager::NotifyConnectionStateChanged(
    mojom::ConnectionState state) const {
  for (auto& obs : observers_) {
    obs.OnConnectionStateChanged(state);
  }
}

}  // namespace brave_vpn
