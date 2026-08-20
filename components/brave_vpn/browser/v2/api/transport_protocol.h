/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_TRANSPORT_PROTOCOL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_TRANSPORT_PROTOCOL_H_

#include <string_view>

#include "base/notreached.h"

namespace brave_vpn::v2::endpoints {

// The VPN tunnel protocols Guardian's device-credentials API supports.
enum class TransportProtocol {
  kIKEv2,
  kWireguard,
};

inline constexpr std::string_view kTransportProtocolIKEv2Value = "ikev2";
inline constexpr std::string_view kTransportProtocolWireguardValue =
    "wireguard";

// Returns the wire-format string Guardian expects for transport-protocol.
inline std::string_view ToTransportProtocolString(
    TransportProtocol transport_protocol) {
  switch (transport_protocol) {
    case TransportProtocol::kIKEv2:
      return kTransportProtocolIKEv2Value;
    case TransportProtocol::kWireguard:
      return kTransportProtocolWireguardValue;
  }
  NOTREACHED();
}

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_TRANSPORT_PROTOCOL_H_
