/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/decentralized_dns_network_delegate_helper.h"

#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_context.h"
#include "net/base/net_errors.h"

namespace decentralized_dns {

int OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK(!next_callback.is_null());

  if (!ctx->browser_context || ctx->browser_context->IsOffTheRecord() ||
      !g_browser_process) {
    return net::OK;
  }

  auto* json_rpc_service =
      brave_wallet::JsonRpcServiceFactory::GetServiceForContext(
          ctx->browser_context);
  if (!json_rpc_service)
    return net::OK;

  if (IsUnstoppableDomainsTLD(ctx->request_url.host_piece()) &&
      IsUnstoppableDomainsResolveMethodEnabled(
          g_browser_process->local_state())) {
    json_rpc_service->UnstoppableDomainsResolveDns(
        ctx->request_url.host(),
        base::BindOnce(&OnBeforeURLRequest_UnstoppableDomainsRedirectWork,
                       next_callback, ctx));

    return net::ERR_IO_PENDING;
  }

  if (IsENSTLD(ctx->request_url.host_piece()) &&
      IsENSResolveMethodEnabled(g_browser_process->local_state())) {
    json_rpc_service->EnsGetContentHash(
        ctx->request_url.host(),
        base::BindOnce(&OnBeforeURLRequest_EnsRedirectWork, next_callback,
                       ctx));

    return net::ERR_IO_PENDING;
  }

  if (base::FeatureList::IsEnabled(
          brave_wallet::features::kBraveWalletSnsFeature) &&
      IsSnsTLD(ctx->request_url.host_piece()) &&
      IsSnsResolveMethodEnabled(g_browser_process->local_state())) {
    json_rpc_service->SnsResolveHost(
        ctx->request_url.host(),
        base::BindOnce(&OnBeforeURLRequest_SnsRedirectWork, next_callback,
                       ctx));

    return net::ERR_IO_PENDING;
  }

  return net::OK;
}

void OnBeforeURLRequest_EnsRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    const std::vector<uint8_t>& content_hash,
    bool require_offchain_consent,
    brave_wallet::mojom::ProviderError error,
    const std::string& error_message) {
  DCHECK(!next_callback.is_null());

  if (error != brave_wallet::mojom::ProviderError::kSuccess) {
    next_callback.Run();
    return;
  }

  if (require_offchain_consent) {
    ctx->pending_error = net::ERR_ENS_OFFCHAIN_LOOKUP_NOT_SELECTED;
    next_callback.Run();
    return;
  }

  GURL ipfs_uri = ipfs::ContentHashToCIDv1URL(content_hash);
  if (ipfs_uri.is_valid()) {
    ctx->new_url_spec = ipfs_uri.spec();
  }

  next_callback.Run();
}

void OnBeforeURLRequest_SnsRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    const absl::optional<GURL>& url,
    brave_wallet::mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error == brave_wallet::mojom::SolanaProviderError::kSuccess && url &&
      url->is_valid()) {
    ctx->new_url_spec = url->spec();
  }

  if (!next_callback.is_null())
    next_callback.Run();
}

void OnBeforeURLRequest_UnstoppableDomainsRedirectWork(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    const GURL& url,
    brave_wallet::mojom::ProviderError error,
    const std::string& error_message) {
  if (error == brave_wallet::mojom::ProviderError::kSuccess && url.is_valid()) {
    ctx->new_url_spec = url.spec();
  }

  if (!next_callback.is_null())
    next_callback.Run();
}

}  // namespace decentralized_dns
