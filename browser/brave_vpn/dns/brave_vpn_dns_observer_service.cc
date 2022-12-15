/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service.h"

#include <vector>

#include "base/strings/string_util.h"
#include "brave/browser/ui/views/brave_vpn/brave_vpn_dns_settings_notificiation_dialog_view.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/simple_message_box.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_vpn {

namespace {
const char kCloudflareDnsProviderURL[] =
    "https://chrome.cloudflare-dns.com/dns-query";

void SkipDNSDialog(PrefService* prefs, bool checked) {
  if (!prefs)
    return;
  prefs->SetBoolean(prefs::kBraveVpnShowDNSPolicyWarningDialog, !checked);
}

gfx::NativeWindow GetAnchorBrowserWindow() {
  auto* browser = chrome::FindLastActive();
  return browser ? browser->window()->GetNativeWindow()
                 : gfx::kNullNativeWindow;
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
  if (state == brave_vpn::mojom::ConnectionState::CONNECTED) {
    LockDNS();
  } else if (state == brave_vpn::mojom::ConnectionState::DISCONNECTED) {
    UnlockDNS();
  }
}

}  // namespace brave_vpn
