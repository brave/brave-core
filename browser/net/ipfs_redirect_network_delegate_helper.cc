/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/ipfs_redirect_network_delegate_helper.h"

#include "brave/components/ipfs/ipfs_gateway.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/translate_ipfs_uri.h"
#include "net/base/net_errors.h"

namespace ipfs {

int OnBeforeURLRequest_IPFSRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  if (!ctx->resolve_ipfs_enabled)
    return net::OK;

  GURL new_url;
  if (ipfs::TranslateIPFSURI(ctx->request_url, &new_url,
                             ctx->ipfs_gateway_url)) {
    // We only allow translating ipfs:// and ipns:// URIs if the initiator_url
    // is from the same Brave ipfs/ipns gateway.
    // For the local case, we don't want a normal site to be able to populate
    // a user's IPFS local cache with content they didn't know about.
    // In which case that user would also be able to serve that content.
    // If the user is not using a local node, we want the experience to be
    // the same as the local case.
    if (ctx->resource_type == blink::mojom::ResourceType::kMainFrame ||
        (IsLocalGatewayURL(new_url) && IsLocalGatewayURL(ctx->initiator_url)) ||
        (IsDefaultGatewayURL(new_url) &&
         IsDefaultGatewayURL(ctx->initiator_url))) {
      ctx->new_url_spec = new_url.spec();
    } else {
      ctx->blocked_by = brave::kOtherBlocked;
    }
  }
  return net::OK;
}

}  // namespace ipfs
