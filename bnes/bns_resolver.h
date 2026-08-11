// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BNES_BNS_RESOLVER_H_
#define BRAVE_BNES_BNS_RESOLVER_H_

#include <string_view>

#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "url/gurl.h"

namespace bnes {

// Resolves a bnes:// URL to an IPFS gateway URL.
//
// The resolver prefers the built-in MetaMask BNS resolver (H1.x). If that is
// not available, it falls back to a direct RPC quorum call (H2.*).
//
// This is a preparation stub for H6.3. The actual implementation depends on
// H5.3 (full build chain) and H1.6 (real-chain contenthash).
void ResolveBnesContent(
    const GURL& url,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client);

// Parses an ENS-style contenthash payload and extracts the CID. Returns true
// if the payload begins with the 0xe3 IPFS namespace tag and the remaining
// bytes decode to a valid CID.
[[nodiscard]] bool ParseContenthash(std::string_view payload, std::string_view& out_cid);

// Validates |gateway_url| is still the trusted origin after redirects.
// Must be re-checked after every redirect hop.
[[nodiscard]] bool IsAllowedGatewayUrl(const GURL& gateway_url);

// Validates a decoded CID and builds a pinned HTTPS gateway URL. Returns false
// if the CID is invalid or the gateway cannot be constructed from the trusted
// host.
[[nodiscard]] bool ValidateAndBuildGateway(std::string_view cid,
                             std::string_view trusted_gateway_host,
                             GURL* out_gateway_url);

}  // namespace bnes

#endif  // BRAVE_BNES_BNS_RESOLVER_H_
