// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_vpn/mac/vpn_utils_mac.h"

#include "brave/components/brave_vpn/browser/connection/ikev2/mac/ikev2_connection_api_impl_mac.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_vpn {

std::unique_ptr<ConnectionAPIImpl> CreateConnectionAPIImplMac(
    BraveVPNConnectionManager* manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // NOTE: WIREGUARD is not supported on macOS yet.
  // See https://github.com/brave/brave-browser/issues/32612
  return std::make_unique<IKEv2ConnectionAPIImplMac>(manager,
                                                     url_loader_factory);
}

}  // namespace brave_vpn
