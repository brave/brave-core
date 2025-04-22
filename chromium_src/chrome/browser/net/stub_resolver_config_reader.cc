/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "base/feature_list.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "build/buildflag.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/network_service_instance.h"
#include "net/dns/public/dns_over_https_config.h"
#include "services/network/public/mojom/network_service.mojom.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/features.h"
#endif

#if BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)
namespace {

bool IsBraveVPNConnected(PrefService* local_state) {
  if (!base::FeatureList::IsEnabled(
          brave_vpn::features::kBraveVPNDnsProtection)) {
    return false;
  }
  return !local_state->GetString(prefs::kBraveVpnDnsConfig).empty();
}

bool ShouldOverride(net::SecureDnsMode secure_dns_mode,
                    PrefService* local_state,
                    SecureDnsConfig::ManagementMode management_mode,
                    bool is_managed) {
  if (!IsBraveVPNConnected(local_state))
    return false;
  if (secure_dns_mode == net::SecureDnsMode::kSecure)
    return false;
  if (management_mode != SecureDnsConfig::ManagementMode::kNoOverride ||
      is_managed) {
    // there is already a managed policy or parental control in place
    return false;
  }

  return true;
}

bool MaybeOverrideDnsClientEnabled(
    net::SecureDnsMode secure_dns_mode,
    bool insecure_dns_client_enabled,
    PrefService* local_state,
    SecureDnsConfig::ManagementMode management_mode,
    bool is_managed) {
  if (ShouldOverride(secure_dns_mode, local_state, management_mode,
                     is_managed)) {
    // disable the insecure client for doh
    return false;
  }

  return insecure_dns_client_enabled;
}

net::SecureDnsMode MaybeOverrideDnsMode(
    net::SecureDnsMode secure_dns_mode,
    PrefService* local_state,
    SecureDnsConfig::ManagementMode management_mode,
    bool is_managed) {
  if (ShouldOverride(secure_dns_mode, local_state, management_mode,
                     is_managed)) {
    return net::SecureDnsMode::kSecure;
  }
  return secure_dns_mode;
}

net::DnsOverHttpsConfig MaybeOverrideDnsConfig(
    net::SecureDnsMode secure_dns_mode,
    net::DnsOverHttpsConfig doh_config,
    PrefService* local_state,
    SecureDnsConfig::ManagementMode management_mode,
    bool is_managed) {
  if (ShouldOverride(secure_dns_mode, local_state, management_mode,
                     is_managed)) {
    return net::DnsOverHttpsConfig::FromStringLax(
        local_state->GetString(prefs::kBraveVpnDnsConfig));
  }
  return doh_config;
}

SecureDnsConfig::ManagementMode MaybeOverrideForcedManagementMode(
    net::SecureDnsMode secure_dns_mode,
    PrefService* local_state,
    SecureDnsConfig::ManagementMode management_mode,
    bool is_managed) {
  // Don't change management mode if the doh settings are
  // managed by policy or by parental controls
  if (is_managed ||
      management_mode != SecureDnsConfig::ManagementMode::kNoOverride)
    return management_mode;

  // Otherwise always block changes to the doh config while the VPN
  // is connected
  if (IsBraveVPNConnected(local_state))
    return SecureDnsConfig::ManagementMode::kDisabledManaged;

  return management_mode;
}

}  // namespace

#define SecureDnsConfig(SECURE_DNS_MODE, SECURE_DOH_CONFIG,                    \
                        FORCED_MANAGEMENT_MODE)                                \
  SecureDnsConfig(                                                             \
      MaybeOverrideDnsMode(SECURE_DNS_MODE, local_state_,                      \
                           FORCED_MANAGEMENT_MODE, is_managed),                \
      MaybeOverrideDnsConfig(SECURE_DNS_MODE, SECURE_DOH_CONFIG, local_state_, \
                             FORCED_MANAGEMENT_MODE, is_managed),              \
      MaybeOverrideForcedManagementMode(SECURE_DNS_MODE, local_state_,         \
                                        FORCED_MANAGEMENT_MODE, is_managed))

#define ConfigureStubHostResolver(                                             \
    INSECURE_DNS_CLIENT_ENABLED, HAPPY_EYEBALLS_V3_ENABLED, SECURE_DNS_MODE,   \
    DNS_OVER_HTTPS_CONFIG, ADDITIONAL_DNS_TYPES_ENABLED)                       \
  ConfigureStubHostResolver(                                                   \
      MaybeOverrideDnsClientEnabled(SECURE_DNS_MODE,                           \
                                    INSECURE_DNS_CLIENT_ENABLED, local_state_, \
                                    forced_management_mode, is_managed),       \
      HAPPY_EYEBALLS_V3_ENABLED,                                               \
      MaybeOverrideDnsMode(SECURE_DNS_MODE, local_state_,                      \
                           forced_management_mode, is_managed),                \
      MaybeOverrideDnsConfig(SECURE_DNS_MODE, DNS_OVER_HTTPS_CONFIG,           \
                             local_state_, forced_management_mode,             \
                             is_managed),                                      \
      ADDITIONAL_DNS_TYPES_ENABLED)
#endif  // BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)
#include "src/chrome/browser/net/stub_resolver_config_reader.cc"
#if BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)
#undef ConfigureStubHostResolver
#undef SecureDnsConfig
#endif  // BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)
