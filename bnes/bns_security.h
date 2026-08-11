// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BNES_BNS_SECURITY_H_
#define BRAVE_BNES_BNS_SECURITY_H_

#include <string_view>

class GURL;

namespace net {
class IPAddress;
}  // namespace net

namespace bnes {

// Validates a raw host string against BNS rules (lowercase, no leading/trailing
// hyphens, valid labels, ends with .bnes, at least one non-empty label before
// the suffix). Does not accept IP addresses or schemes.
[[nodiscard]] bool IsValidBnesHost(std::string_view host);

// Validates user-entered navigation before any resolver or gateway request.
[[nodiscard]] bool IsAllowedNavigationUrl(const GURL& url);

// Validates a configured IPFS gateway against an explicit trusted-host list.
// Gateway redirects must be checked again after every hop with this function.
[[nodiscard]] bool IsAllowedGatewayUrl(const GURL& url,
                         std::string_view trusted_gateway_host);

// Must be applied to every resolved address immediately before connection. DNS
// names are not sufficient: an attacker can rebind a permitted hostname.
[[nodiscard]] bool IsPublicNetworkAddress(const net::IPAddress& address);

// CID is deliberately restricted to the text representations accepted by the
// configured path gateway. It is never interpolated as an arbitrary URL.
[[nodiscard]] bool IsValidCid(std::string_view cid);

}  // namespace bnes

#endif  // BRAVE_BNES_BNS_SECURITY_H_
