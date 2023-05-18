/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_connection_api.h"

#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;

std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel) {
  return std::make_unique<BraveVPNWireguardConnectionAPI>(url_loader_factory,
                                                          local_prefs, channel);
}

// TODO(spylogsster): Implement wireguard connection using Wireguard service.
BraveVPNWireguardConnectionAPI::BraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel)
    : BraveVPNOSConnectionAPI(url_loader_factory, local_prefs),
      local_prefs_(local_prefs),
      url_loader_factory_(url_loader_factory) {
  DCHECK(url_loader_factory_ && local_prefs_);
}

BraveVPNWireguardConnectionAPI::~BraveVPNWireguardConnectionAPI() {}

std::string BraveVPNWireguardConnectionAPI::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void BraveVPNWireguardConnectionAPI::Connect() {}

void BraveVPNWireguardConnectionAPI::Disconnect() {}

void BraveVPNWireguardConnectionAPI::ToggleConnection() {}

void BraveVPNWireguardConnectionAPI::CheckConnection() {}

void BraveVPNWireguardConnectionAPI::ResetConnectionInfo() {}

std::string BraveVPNWireguardConnectionAPI::GetHostname() const {
  return std::string();
}

void BraveVPNWireguardConnectionAPI::SetSelectedRegion(
    const std::string& name) {}

}  // namespace brave_vpn
