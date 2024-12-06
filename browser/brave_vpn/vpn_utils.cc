/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/vpn_utils.h"

#include "base/functional/bind.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_vpn/win/vpn_utils_win.h"
#include "brave/browser/brave_vpn/win/wireguard_connection_api_impl_win.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/win/ras_connection_api_impl_win.h"
#endif

#if BUILDFLAG(IS_MAC)
#include "brave/browser/brave_vpn/mac/vpn_utils_mac.h"
#endif

namespace brave_vpn {

namespace {

#if !BUILDFLAG(IS_ANDROID)
std::unique_ptr<ConnectionAPIImpl> CreateConnectionAPIImpl(
    BraveVPNConnectionManager* manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    bool wireguard_enabled) {
#if BUILDFLAG(IS_MAC)
  return CreateConnectionAPIImplMac(manager, url_loader_factory);
#endif

#if BUILDFLAG(IS_WIN)
  if (wireguard_enabled) {
    return std::make_unique<WireguardConnectionAPIImplWin>(manager,
                                                           url_loader_factory);
  }
  return std::make_unique<RasConnectionAPIImplWin>(manager, url_loader_factory);
#endif

  // VPN is not supported on other platforms.
  NOTREACHED();
}
#endif

}  // namespace

std::unique_ptr<BraveVPNConnectionManager> CreateBraveVPNConnectionManager(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs) {
#if BUILDFLAG(IS_ANDROID)
  // Android doesn't use connection api.
  return nullptr;
#else
  // Currently, service installer only used on Windows.
  // Installs registers IKEv2 service (for DNS) and our WireGuard impl.
  // NOTE: Install only happens if person has purchased the product.
  auto service_installer =
#if BUILDFLAG(IS_WIN)
      base::BindRepeating(&brave_vpn::InstallVpnSystemServices);
#else
      base::NullCallback();
#endif

  auto manager = std::make_unique<BraveVPNConnectionManager>(
      url_loader_factory, local_prefs, service_installer);
  manager->set_target_vpn_entry_name(
      brave_vpn::GetBraveVPNEntryName(chrome::GetChannel()));
  manager->set_connection_api_impl_getter(
      base::BindRepeating(&CreateConnectionAPIImpl));
  manager->UpdateConnectionAPIImpl();
  return manager;
#endif
}

bool IsAllowedForContext(content::BrowserContext* context) {
  return Profile::FromBrowserContext(context)->IsRegularProfile() &&
         brave_vpn::IsBraveVPNFeatureEnabled();
}

bool IsBraveVPNEnabled(content::BrowserContext* context) {
  // TODO(simonhong): Can we use this check for android?
  // For now, vpn is disabled by default on desktop but not sure on
  // android.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  return brave_vpn::IsBraveVPNEnabled(user_prefs::UserPrefs::Get(context)) &&
         IsAllowedForContext(context);
#else
  return Profile::FromBrowserContext(context)->IsRegularProfile();
#endif
}

}  // namespace brave_vpn
