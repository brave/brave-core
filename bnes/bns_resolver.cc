// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/bnes/bns_resolver.h"

#include <string_view>

#include "base/check.h"
#include "brave/bnes/bns_security.h"
#include "brave/bnes/bns_constants.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace bnes {

void ResolveBnesContent(
    const GURL& url,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client) {
  DCHECK(IsAllowedNavigationUrl(url));

  // Extract the host (e.g., "bear.bnes").
  const std::string host = url.host();

  // TODO(H6.3): Implement the actual resolution pipeline:
  // 1. Try built-in MetaMask BNS resolver (H1.x).
  // 2. Fallback to direct RPC quorum (H2.*).
  // 3. Validate returned CID via IsValidCid().
  // 4. Build gateway URL: https://ipfs.bearnetwork.net/ipfs/<CID>
  // 5. Verify gateway via IsAllowedGatewayUrl().
  // 6. Stream content to client.

  mojo::Remote<network::mojom::URLLoaderClient> client_remote(
      std::move(client));
  client_remote->OnComplete(
      network::URLLoaderCompletionStatus(net::ERR_NOT_IMPLEMENTED));
}

bool ParseContenthash(std::string_view payload, std::string_view& out_cid) {
  // ENS IPFS contenthash encoding: 0xe3 || <varint prefix> || <CID bytes>.
  // We only accept the bare CID bytes after the 0xe3 tag and validate them
  // against the same rules as user-supplied CIDs.
  constexpr std::string_view kIpfsNamespaceTag = "\xe3";
  if (payload.size() < kIpfsNamespaceTag.size() ||
      !payload.starts_with(kIpfsNamespaceTag)) {
    return false;
  }

  std::string_view cid = payload.substr(kIpfsNamespaceTag.size());
  if (!IsValidCid(cid)) {
    return false;
  }

  out_cid = cid;
  return true;
}

bool IsAllowedGatewayUrl(const GURL& gateway_url) {
  return bnes::IsAllowedGatewayUrl(gateway_url, kDefaultIpfsGatewayHost);
}

bool ValidateAndBuildGateway(std::string_view cid,
                             std::string_view trusted_gateway_host,
                             GURL* out_gateway_url) {
  if (!IsValidCid(cid) || trusted_gateway_host.empty() || !out_gateway_url) {
    return false;
  }

  std::string gateway =
      std::string(url::kHttpsScheme).append("://")
          .append(trusted_gateway_host)
          .append(kIpfsPathPrefix)
          .append(cid);

  GURL url(gateway);
  if (!url.is_valid() || !IsAllowedGatewayUrl(url, trusted_gateway_host)) {
    return false;
  }

  *out_gateway_url = url;
  return true;
}

}  // namespace bnes
