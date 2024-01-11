/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service_win.h"

#include <vector>

#include "base/strings/string_util.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"
#include "brave/browser/ui/views/brave_vpn/brave_vpn_dns_settings_notificiation_dialog_view.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/brave_vpn/common/win/utils.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/simple_message_box.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/branded_strings.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_vpn {

namespace {
constexpr char kCloudflareDnsProviderURL[] =
    "https://chrome.cloudflare-dns.com/dns-query";

// Timer to recheck the service launch after some time and fallback to DoH if
// the service was not launched.
constexpr int kHelperServiceStartTimeoutSec = 5;

void SkipDNSDialog(PrefService* prefs, bool checked) {
  if (!prefs)
    return;
  prefs->SetBoolean(prefs::kBraveVpnShowDNSPolicyWarningDialog, !checked);
}

gfx::NativeWindow GetAnchorBrowserWindow() {
  auto* browser = chrome::FindLastActive();
  return browser ? browser->window()->GetNativeWindow() : gfx::NativeWindow();
}

bool AreConfigsEqual(SecureDnsConfig& one, SecureDnsConfig& two) {
  return one.mode() == two.mode() &&
         one.management_mode() == two.management_mode() &&
         one.doh_servers() == two.doh_servers();
}

}  // namespace

BraveVpnDnsObserverService::BraveVpnDnsObserverService(
    PrefService* local_state,
    PrefService* profile_prefs)
    : local_state_(local_state), profile_prefs_(profile_prefs) {
  DCHECK(profile_prefs_);
  DCHECK(local_state_);
  local_state_->ClearPref(::prefs::kBraveVpnDnsConfig);
}

BraveVpnDnsObserverService::~BraveVpnDnsObserverService() = default;

void BraveVpnDnsObserverService::ShowPolicyWarningMessage() {
  if (!profile_prefs_->GetBoolean(prefs::kBraveVpnShowDNSPolicyWarningDialog)) {
    return;
  }

  if (policy_callback_) {
    std::move(policy_callback_).Run();
    return;
  }

  chrome::ShowWarningMessageBoxWithCheckbox(
      GetAnchorBrowserWindow(), l10n_util::GetStringUTF16(IDS_PRODUCT_NAME),
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_DNS_POLICY_ALERT),
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_DNS_POLICY_CHECKBOX),
      base::BindOnce(&SkipDNSDialog, profile_prefs_));
}

void BraveVpnDnsObserverService::ShowVpnDnsSettingsNotificationDialog() {
  if (dialog_callback_) {
    dialog_callback_.Run();
    return;
  }
  BraveVpnDnsSettingsNotificiationDialogView::Show(chrome::FindLastActive());
}

void BraveVpnDnsObserverService::UnlockDNS() {
  local_state_->ClearPref(::prefs::kBraveVpnDnsConfig);
  // Read DNS config to initiate update of actual state.
  SystemNetworkContextManager::GetStubResolverConfigReader()
      ->UpdateNetworkService(false);
}

bool BraveVpnDnsObserverService::IsDNSHelperLive() {
  if (dns_helper_live_for_testing_.has_value()) {
    return dns_helper_live_for_testing_.value();
  }
  // If the service is not installed we should override DNS because it will be
  // handled by the service.
  if (!IsBraveVPNHelperServiceInstalled()) {
    return false;
  }

  if (IsWindowsServiceRunning(brave_vpn::GetBraveVpnHelperServiceName())) {
    RunServiceWatcher();
  }

  if (IsNetworkFiltersInstalled()) {
    return true;
  }

  // The service can be stopped and this is valid state, not started yet,
  // crashed once and restarting and so on.
  content::GetUIThreadTaskRunner({})->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveVpnDnsObserverService::OnCheckIfServiceStarted,
                     weak_ptr_factory_.GetWeakPtr()),
      base::Seconds(kHelperServiceStartTimeoutSec));
  return true;
}

void BraveVpnDnsObserverService::RunServiceWatcher() {
  service_watcher_.reset(new brave::ServiceWatcher());
  if (!service_watcher_->Subscribe(
          brave_vpn::GetBraveVpnHelperServiceName(), SERVICE_NOTIFY_STOPPED,
          base::BindRepeating(&BraveVpnDnsObserverService::OnServiceStopped,
                              weak_ptr_factory_.GetWeakPtr()))) {
    VLOG(1) << "Unable to set service watcher";
  }
}

void BraveVpnDnsObserverService::OnServiceStopped(int mask) {
  // Postpone check because the service can be restarted by the system due to
  // configured failure actions.
  content::GetUIThreadTaskRunner({})->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveVpnDnsObserverService::OnCheckIfServiceStarted,
                     weak_ptr_factory_.GetWeakPtr()),
      base::Seconds(kHelperServiceStartTimeoutSec));
}

bool BraveVpnDnsObserverService::IsVPNConnected() const {
  return connection_state_.has_value() &&
         connection_state_.value() ==
             brave_vpn::mojom::ConnectionState::CONNECTED;
}

void BraveVpnDnsObserverService::OnCheckIfServiceStarted() {
  if (!IsVPNConnected()) {
    return;
  }
  // Checking if the service was not launched or filters were not set.
  if (!IsNetworkFiltersInstalled() ||
      !IsWindowsServiceRunning(brave_vpn::GetBraveVpnHelperServiceName())) {
    LockDNS();
    return;
  }
  RunServiceWatcher();
}

void BraveVpnDnsObserverService::LockDNS() {
  auto old_dns_config =
      SystemNetworkContextManager::GetStubResolverConfigReader()
          ->GetSecureDnsConfiguration(false);

  local_state_->SetString(::prefs::kBraveVpnDnsConfig,
                          kCloudflareDnsProviderURL);

  // Trigger StubResolverConfigReader to see if it should override the settings
  // with kBraveVpnDnsConfig
  SystemNetworkContextManager::GetStubResolverConfigReader()
      ->UpdateNetworkService(false);
  auto new_dns_config =
      SystemNetworkContextManager::GetStubResolverConfigReader()
          ->GetSecureDnsConfiguration(false);

  if (old_dns_config.mode() != net::SecureDnsMode::kSecure) {
    if (AreConfigsEqual(old_dns_config, new_dns_config)) {
      ShowPolicyWarningMessage();
    } else {
      ShowVpnDnsSettingsNotificationDialog();
    }
  }
}

void BraveVpnDnsObserverService::OnConnectionStateChanged(
    brave_vpn::mojom::ConnectionState state) {
  connection_state_ = state;
  if (state == brave_vpn::mojom::ConnectionState::CONNECTED) {
    if (IsDNSHelperLive()) {
      return;
    }
    LockDNS();
  } else if (state == brave_vpn::mojom::ConnectionState::DISCONNECTED) {
    UnlockDNS();
  }
}

}  // namespace brave_vpn
