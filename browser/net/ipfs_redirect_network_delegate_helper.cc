/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/ipfs_redirect_network_delegate_helper.h"

#include <string>

#include "brave/browser/profiles/profile_util.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/common/channel_info.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"

namespace ipfs {

int OnBeforeURLRequest_IPFSRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  const bool has_ipfs_scheme = IsIPFSScheme(ctx->request_url);
  if (!ctx->browser_context) {
    // IPFS url translation depends on selected gateway.
    // So we block IPFS requests if we don't have access to prefs.
    if (has_ipfs_scheme) {
      ctx->blocked_by = brave::kOtherBlocked;
    }
    return net::OK;
  }

  auto* prefs = user_prefs::UserPrefs::Get(ctx->browser_context);
  const bool ipfs_disabled = IsIpfsResolveMethodDisabled(prefs);

  if (has_ipfs_scheme && !brave::IsRegularProfile(ctx->browser_context)) {
    // Don't allow IPFS requests without translation of IPFS urls.
    ctx->blocked_by = brave::kOtherBlocked;
    // Only net::OK navigation will be actually blocked without commit.
    // Show proper error for mainframe navigation.
    return ctx->resource_type == blink::mojom::ResourceType::kMainFrame
               ? net::ERR_INCOGNITO_IPFS_NOT_ALLOWED
               : net::OK;
  }

  if (has_ipfs_scheme && ipfs_disabled) {
    ctx->blocked_by = brave::kOtherBlocked;
    // Only net::OK navigation will be actually blocked without commit.
    // Show proper error for mainframe navigation.
    return ctx->resource_type == blink::mojom::ResourceType::kMainFrame
               ? net::ERR_IPFS_DISABLED
               : net::OK;
  }

  if (has_ipfs_scheme && IsIpfsResolveMethodAsk(prefs)) {
    ctx->blocked_by = brave::kOtherBlocked;
    // Only net::OK navigation will be actually blocked without commit.
    // Show proper error for mainframe navigation.
    return ctx->resource_type == blink::mojom::ResourceType::kMainFrame
               ? net::ERR_IPFS_RESOLVE_METHOD_NOT_SELECTED
               : net::OK;
  }

  LOG(INFO) << "[IPFS] OnBeforeURLRequest URL:" << ctx->request_url;
  GURL new_url;
  if (ipfs::TranslateIPFSURI(ctx->request_url, &new_url, ctx->ipfs_gateway_url,
                             false)) {
  LOG(INFO) << "[IPFS] OnBeforeURLRequest #1 new_url:" << new_url;
    // We only allow translating ipfs:// and ipns:// URIs if the initiator_url
    // is from the same Brave ipfs/ipns gateway.
    // For the local case, we don't want a normal site to be able to populate
    // a user's IPFS local cache with content they didn't know about.
    // In which case that user would also be able to serve that content.
    // If the user is not using a local node, we want the experience to be
    // the same as the local case.
    if (ctx->resource_type == blink::mojom::ResourceType::kMainFrame ||
        (IsLocalGatewayURL(new_url) && IsLocalGatewayURL(ctx->initiator_url)) ||
        (IsDefaultGatewayURL(new_url, prefs) &&
         IsDefaultGatewayURL(ctx->initiator_url, prefs))) {
      ctx->new_url_spec = new_url.spec();
    } else {
      ctx->blocked_by = brave::kOtherBlocked;
    }
  } else if (has_ipfs_scheme) {
  LOG(INFO) << "[IPFS] OnBeforeURLRequest #2 new_url:" << new_url;
    // Block incorrect url.
    ctx->blocked_by = brave::kOtherBlocked;
  }

  return net::OK;
}

}  // namespace ipfs
