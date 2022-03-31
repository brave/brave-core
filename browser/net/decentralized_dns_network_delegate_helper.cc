/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/decentralized_dns_network_delegate_helper.h"

#include <utility>
#include <vector>

#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/utils.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_context.h"
#include "net/base/net_errors.h"

namespace decentralized_dns {

namespace {

std::string GetValue(const std::vector<std::string>& arr, RecordKeys key) {
  return arr[static_cast<size_t>(key)];
}

}  // namespace

int OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  if (!ctx->browser_context || !IsDecentralizedDnsEnabled() ||
      ctx->browser_context->IsOffTheRecord() || !g_browser_process) {
    return net::OK;
  }

  auto* json_rpc_service =
      brave_wallet::JsonRpcServiceFactory::GetServiceForContext(
          ctx->browser_context);
  if (!json_rpc_service)
    return net::OK;

  if (IsUnstoppableDomainsTLD(ctx->request_url) &&
      IsUnstoppableDomainsResolveMethodEthereum(
          g_browser_process->local_state())) {
    auto keys = std::vector<std::string>(std::begin(kRecordKeys),
                                         std::end(kRecordKeys));
    json_rpc_service->UnstoppableDomainsProxyReaderGetMany(
        brave_wallet::mojom::kMainnetChainId, ctx->request_url.host(), keys,
        base::BindOnce(&OnBeforeURLRequest_UnstoppableDomainsRedirectWork,
                       next_callback, ctx));

    return net::ERR_IO_PENDING;
  }

  if (IsENSTLD(ctx->request_url) &&
      IsENSResolveMethodEthereum(g_browser_process->local_state())) {
    json_rpc_service->EnsResolverGetContentHash(
        brave_wallet::mojom::kMainnetChainId, ctx->request_url.host(),
        base::BindOnce(&OnBeforeURLRequest_EnsRedirectWork, next_callback,
                       ctx));

    return net::ERR_IO_PENDING;
  }

  return net::OK;
}

void OnBeforeURLRequest_EnsRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    const std::string& content_hash,
    brave_wallet::mojom::ProviderError error,
    const std::string& error_message) {
  if (error != brave_wallet::mojom::ProviderError::kSuccess) {
    if (!next_callback.is_null())
      next_callback.Run();
    return;
  }

  GURL ipfs_uri = ipfs::ContentHashToCIDv1URL(content_hash);
  if (ipfs_uri.is_valid()) {
    ctx->new_url_spec = ipfs_uri.spec();
  }

  if (!next_callback.is_null())
    next_callback.Run();
}

void OnBeforeURLRequest_UnstoppableDomainsRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    const std::vector<std::string>& values,
    brave_wallet::mojom::ProviderError error,
    const std::string& error_message) {
  if (error != brave_wallet::mojom::ProviderError::kSuccess ||
      values.size() != static_cast<size_t>(RecordKeys::MAX_RECORD_KEY) + 1) {
    if (!next_callback.is_null())
      next_callback.Run();
    return;
  }

  // Redirect to ipfs URI if content hash is set, otherwise, fallback to the
  // set redirect URL. If no records available to use, do nothing. See
  // https://docs.unstoppabledomains.com/browser-resolution/browser-resolution-algorithm
  // for more details.
  //
  // TODO(jocelyn): Do not fallback to the set redirect URL if dns.A or
  // dns.AAAA is not empty once we support the classical DNS records case.
  std::string ipfs_uri = GetValue(values, RecordKeys::DWEB_IPFS_HASH);
  if (ipfs_uri.empty()) {  // Try legacy value.
    ipfs_uri = GetValue(values, RecordKeys::IPFS_HTML_VALUE);
  }

  std::string fallback_url = GetValue(values, RecordKeys::BROWSER_REDIRECT_URL);
  if (fallback_url.empty()) {  // Try legacy value.
    fallback_url = GetValue(values, RecordKeys::IPFS_REDIRECT_DOMAIN_VALUE);
  }

  if (!ipfs_uri.empty()) {
    ctx->new_url_spec = GURL("ipfs://" + ipfs_uri).spec();
  } else if (!fallback_url.empty()) {
    ctx->new_url_spec = GURL(fallback_url).spec();
  }

  if (!next_callback.is_null())
    next_callback.Run();
}

}  // namespace decentralized_dns
