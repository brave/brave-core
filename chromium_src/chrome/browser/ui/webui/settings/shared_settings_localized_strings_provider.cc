/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/fixed_flat_map.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/net/dns/secure_dns_endpoints.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "net/base/features.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/features.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

#if BUILDFLAG(ENABLE_BRAVE_VPN) && BUILDFLAG(IS_WIN)
namespace {

bool ShouldReplaceSecureDNSDisabledDescription() {
  if (!base::FeatureList::IsEnabled(
          brave_vpn::features::kBraveVPNDnsProtection))
    return false;
  auto dns_config = SystemNetworkContextManager::GetStubResolverConfigReader()
                        ->GetSecureDnsConfiguration(false);
  return !g_browser_process->local_state()
              ->GetString(prefs::kBraveVpnDnsConfig)
              .empty() ||
         dns_config.mode() == net::SecureDnsMode::kSecure;
}

}  // namespace

#endif  // BUILDFLAG(ENABLE_BRAVE_VPN) && BUILDFLAG(IS_WIN)

#define AddSecureDnsStrings AddSecureDnsStrings_ChromiumImpl

#include "src/chrome/browser/ui/webui/settings/shared_settings_localized_strings_provider.cc"

#undef AddSecureDnsStrings
namespace settings {

void AddSecureDnsStrings(content::WebUIDataSource* html_source) {
  AddSecureDnsStrings_ChromiumImpl(html_source);
  if (base::FeatureList::IsEnabled(net::features::kBraveFallbackDoHProvider)) {
    static const net::DohFallbackEndpointType endpoint =
        net::features::kBraveFallbackDoHProviderEndpoint.Get();
    if (endpoint != net::DohFallbackEndpointType::kNone) {
      static constexpr auto kAlternateLocalizedStrings =
          base::MakeFixedFlatMap<net::DohFallbackEndpointType, int>({
              {net::DohFallbackEndpointType::kQuad9,
               IDS_SETTINGS_AUTOMATIC_MODE_WITH_QUAD9_DESCRIPTION},
              {net::DohFallbackEndpointType::kWikimedia,
               IDS_SETTINGS_AUTOMATIC_MODE_WITH_WIKIMEDIA_DESCRIPTION},
              {net::DohFallbackEndpointType::kCloudflare,
               IDS_SETTINGS_AUTOMATIC_MODE_WITH_CLOUDFLARE_DESCRIPTION},
          });
      html_source->AddLocalizedString("secureDnsAutomaticModeDescription",
                                      kAlternateLocalizedStrings.at(endpoint));
    }
  }
#if BUILDFLAG(ENABLE_BRAVE_VPN) && BUILDFLAG(IS_WIN)
  if (ShouldReplaceSecureDNSDisabledDescription()) {
    static constexpr webui::LocalizedString kLocalizedStrings[] = {
        {"secureDnsDisabledForManagedEnvironment",
         IDS_SETTINGS_SECURE_DNS_DISABLED_BY_BRAVE_VPN}};
    html_source->AddLocalizedStrings(kLocalizedStrings);
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN) && BUILDFLAG(IS_WIN)
}

}  // namespace settings

