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

  GURL new_url;
  if (ipfs::TranslateIPFSURI(ctx->request_url, &new_url, ctx->ipfs_gateway_url,
                             false)) {
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
    // Block incorrect url.
    ctx->blocked_by = brave::kOtherBlocked;
  }

  return net::OK;
}

int OnHeadersReceived_IPFSRedirectWork(
    const net::HttpResponseHeaders* response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  if (!ctx->browser_context)
    return net::OK;
  auto* prefs = user_prefs::UserPrefs::Get(ctx->browser_context);
  if (IsIpfsResolveMethodDisabled(prefs)) {
    return net::OK;
  }

  std::string ipfs_path;
  bool api_gateway = IsAPIGateway(ctx->request_url, chrome::GetChannel());
  if (ctx->ipfs_auto_fallback && !api_gateway && response_headers &&
      response_headers->GetNormalizedHeader("x-ipfs-path", &ipfs_path) &&
      // Make sure we don't infinite redirect
      !ctx->request_url.DomainIs(ctx->ipfs_gateway_url.host()) &&
      // Do not redirect if the frame is not ipfs/ipns
      IsIPFSScheme(ctx->initiator_url)) {
    GURL::Replacements replacements;
    replacements.SetPathStr(ipfs_path);

    if (ctx->request_url.has_query()) {
      replacements.SetQueryStr(ctx->request_url.query_piece());
    }

    GURL new_url = ctx->ipfs_gateway_url.ReplaceComponents(replacements);

    *override_response_headers =
        new net::HttpResponseHeaders(response_headers->raw_headers());
    (*override_response_headers)
        ->ReplaceStatusLine("HTTP/1.1 307 Temporary Redirect");
    (*override_response_headers)->RemoveHeader("Location");
    (*override_response_headers)->AddHeader("Location", new_url.spec());
    *allowed_unsafe_redirect_url = new_url;
  }

  return net::OK;
}

}  // namespace ipfs
