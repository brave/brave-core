// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/bnes/bns_scheme_handler.h"

#include <string_view>

#include "base/check.h"
#include "base/memory/self_deleting.h"
#include "brave/bnes/bns_resolver.h"
#include "brave/bnes/bns_security.h"
#include "brave/bnes/bns_constants.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "url/gurl.h"

namespace bnes {

namespace {

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("bnes_scheme_handler", R"(
        semantics {
          description: "Native BNS navigation for bnes:// URLs."
          trigger: "User enters a bnes:// URL or clicks a bnes:// link."
          data: "The bnes:// hostname (e.g., bear.bnes)."
        }
        policy {
          cookies_allowed: false
          setting: "This feature is part of BnesBrowser native routing."
        }
      )");

}  // namespace

// static
mojo::PendingRemote<network::mojom::URLLoaderFactory>
BnesURLLoaderFactory::Create() {
  mojo::PendingRemote<network::mojom::URLLoaderFactory> pending_remote;

  base::MakeSelfDeleting<BnesURLLoaderFactory>(
      pending_remote.InitWithNewPipeAndPassReceiver());

  return pending_remote;
}

BnesURLLoaderFactory::BnesURLLoaderFactory(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver,
    base::SelfDeletingPassKey key)
    : network::SelfDeletingURLLoaderFactory(std::move(factory_receiver), key) {}

BnesURLLoaderFactory::~BnesURLLoaderFactory() = default;

void BnesURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  if (!request.url.SchemeIs(kBnesScheme)) {
    return;
  }

  if (!IsAllowedNavigationUrl(request.url)) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_INVALID_URL));
    return;
  }

  StartAsyncResolve(request, std::move(client));
}

void BnesURLLoaderFactory::StartAsyncResolve(
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client) {
  DCHECK(request.url.SchemeIs(kBnesScheme));
  DCHECK(IsAllowedNavigationUrl(request.url));

  if (!IsAllowedNavigationUrl(request.url)) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_INVALID_URL));
    return;
  }

  ResolveBnesContent(request.url, std::move(client));
}

}  // namespace bnes
