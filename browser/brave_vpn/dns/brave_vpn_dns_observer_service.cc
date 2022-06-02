/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service.h"

#include "base/bind.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/secure_dns_util.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/simple_message_box.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "components/country_codes/country_codes.h"
#include "components/grit/brave_components_strings.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "net/dns/public/dns_over_https_config.h"
#include "net/dns/public/doh_provider_entry.h"
#include "net/dns/public/secure_dns_mode.h"
#include "ui/base/l10n/l10n_util.h"

namespace secure_dns = chrome_browser_net::secure_dns;

namespace brave_vpn {

namespace {
const char kBraveVpnDnsProvider[] = "Cloudflare";
void SkipDNSDialog(PrefService* prefs, bool checked) {
  if (!prefs)
    return;
  prefs->SetBoolean(prefs::kBraveVPNShowDNSPolicyWarningDialog, !checked);
}
std::string GetFilteredProvidersForCountry() {
  namespace secure_dns = chrome_browser_net::secure_dns;
  // Use default hardcoded servers for current country.
  auto providers = secure_dns::ProvidersForCountry(
      secure_dns::SelectEnabledProviders(net::DohProviderEntry::GetList()),
      country_codes::GetCurrentCountryID());
  for (const net::DohProviderEntry* entry : net::DohProviderEntry::GetList()) {
    if (entry->provider != kBraveVpnDnsProvider)
      continue;
    net::DnsOverHttpsConfig doh_config({entry->doh_server_config});
    return doh_config.ToString();
  }
  NOTREACHED() << "Should not be reached as we expect " << kBraveVpnDnsProvider
               << " is available in the default list.";
  return std::string("1.1.1.1");
}
std::string GetDoHServers(SecureDnsConfig* dns_config) {
  return dns_config && !dns_config->doh_servers().servers().empty()
             ? dns_config->doh_servers().ToString()
             : GetFilteredProvidersForCountry();
}
gfx::NativeWindow GetAnchorBrowserWindow() {
  auto* browser = chrome::FindLastActive();
  return browser ? browser->window()->GetNativeWindow()
                 : gfx::kNullNativeWindow;
}
}  // namespace

BraveVpnDnsObserverService::BraveVpnDnsObserverService(
    PrefService* local_state,
    DnsPolicyReaderCallback callback)
    : policy_reader_(std::move(callback)), local_state_(local_state) {
  pref_change_registrar_.Init(local_state);
  pref_change_registrar_.Add(
      ::prefs::kDnsOverHttpsMode,
      base::BindRepeating(&BraveVpnDnsObserverService::OnDNSPrefChanged,
                          weak_ptr_factory_.GetWeakPtr()));
}

BraveVpnDnsObserverService::~BraveVpnDnsObserverService() = default;

bool BraveVpnDnsObserverService::ShouldAllowExternalChanges() const {
  if (allow_changes_for_testing_.has_value())
    return allow_changes_for_testing_.value();

  return (chrome::ShowQuestionMessageBoxSync(
              GetAnchorBrowserWindow(),
              l10n_util::GetStringUTF16(IDS_PRODUCT_NAME),
              l10n_util::GetStringUTF16(IDS_BRAVE_VPN_DNS_CHANGE_ALERT)) ==
          chrome::MESSAGE_BOX_RESULT_YES);
}

bool BraveVpnDnsObserverService::IsDnsModeConfiguredByPolicy() const {
  return policy_reader_ &&
         !policy_reader_.Run(policy::key::kDnsOverHttpsMode).empty();
}

PrefService* BraveVpnDnsObserverService::GetPrefService() {
  if (pref_service_for_testing_)
    return pref_service_for_testing_;
  auto* browser = chrome::FindLastActive();
  if (!browser)
    return nullptr;
  return browser->profile()->GetPrefs();
}

void BraveVpnDnsObserverService::ShowPolicyWarningMessage() {
  auto* prefs = GetPrefService();
  if (prefs && !prefs->GetBoolean(prefs::kBraveVPNShowDNSPolicyWarningDialog)) {
    return;
  }

#if !defined(OFFICIAL_BUILD)
  if (policy_callback_) {
    std::move(policy_callback_).Run();
    return;
  }
#endif
  chrome::ShowWarningMessageBoxWithCheckbox(
      GetAnchorBrowserWindow(), l10n_util::GetStringUTF16(IDS_PRODUCT_NAME),
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_DNS_POLICY_ALERT),
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_DNS_POLICY_CHECKBOX),
      base::BindOnce(&SkipDNSDialog, prefs));
}

void BraveVpnDnsObserverService::OnDNSPrefChanged() {
  if (ignore_prefs_change_)
    return;
  // Reset saved config and keep user's choice.
  if (ShouldAllowExternalChanges()) {
    user_dns_config_.reset();
  } else {
    ignore_prefs_change_ = true;
    SetDNSOverHTTPSMode(SecureDnsConfig::kModeSecure,
                        GetDoHServers(user_dns_config_.get()));
    ignore_prefs_change_ = false;
  }
}

void BraveVpnDnsObserverService::SetDNSOverHTTPSMode(
    const std::string& mode,
    const std::string& doh_providers) {
  local_state_->SetString(::prefs::kDnsOverHttpsTemplates, doh_providers);
  local_state_->SetString(::prefs::kDnsOverHttpsMode, mode);
}

void BraveVpnDnsObserverService::OnConnectionStateChanged(
    brave_vpn::mojom::ConnectionState state) {
  if (state == brave_vpn::mojom::ConnectionState::CONNECTED) {
    auto dns_config = std::make_unique<SecureDnsConfig>(
        SystemNetworkContextManager::GetStubResolverConfigReader()
            ->GetSecureDnsConfiguration(false));
    if (local_state_->GetString(::prefs::kDnsOverHttpsMode) !=
        SecureDnsConfig::kModeSecure) {
      // If DNS mode configured by policies we notify user that DNS may leak
      // via configured DNS gateway.
      if (IsDnsModeConfiguredByPolicy()) {
        ShowPolicyWarningMessage();
        return;
      }
      SetDNSOverHTTPSMode(SecureDnsConfig::kModeSecure,
                          GetDoHServers(dns_config.get()));
    }
    user_dns_config_ = std::move(dns_config);
    ignore_prefs_change_ = false;
  } else if (user_dns_config_) {
    ignore_prefs_change_ = true;
    auto* mode_to_restore =
        SecureDnsConfig::ModeToString(user_dns_config_->mode());
    auto servers = user_dns_config_->doh_servers().ToString();
    user_dns_config_.reset();
    SetDNSOverHTTPSMode(mode_to_restore, servers);
  }
}

}  // namespace brave_vpn
