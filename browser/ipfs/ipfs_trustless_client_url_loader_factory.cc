/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_trustless_client_url_loader_factory.h"
#include "services/network/public/cpp/resource_request.h"

namespace ipfs {

//static
void IpfsTrustlessClientUrlLoaderFactory::Create(
    std::map<std::string,
               mojo::PendingRemote<network::mojom::URLLoaderFactory>>* in_out,
    content::BrowserContext* context,
    URLLoaderFactory* default_factory,
    network::mojom::NetworkContext* net_ctxt) {
  for (char const* scheme : {"ipfs", "ipns"}) {
    mojo::PendingRemote<network::mojom::URLLoaderFactory> pending;
    new IpfsTrustlessClientUrlLoaderFactory(scheme, pending.InitWithNewPipeAndPassReceiver(),
                             context, default_factory, net_ctxt);
    in_out->emplace(scheme, std::move(pending));
  }
}


IpfsTrustlessClientUrlLoaderFactory::IpfsTrustlessClientUrlLoaderFactory(
    std::string scheme,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver,
    content::BrowserContext* context,
    URLLoaderFactory* default_factory,
    network::mojom::NetworkContext* net_ctxt)
    : network::SelfDeletingURLLoaderFactory(std::move(factory_receiver)) {

    }

IpfsTrustlessClientUrlLoaderFactory::~IpfsTrustlessClientUrlLoaderFactory() =
    default;


void IpfsTrustlessClientUrlLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t /*request_id*/,
    uint32_t /*options*/,
    network::ResourceRequest const& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    net::MutableNetworkTrafficAnnotationTag const&  // traffic_annotation
) {
  VLOG(2) << "IPFS subresource: case=" << scheme_
          << " url=" << request.url.spec();
  DCHECK(default_factory_);
  if (scheme_ == "ipfs" || scheme_ == "ipns") {
    // auto ptr = std::make_shared<IpfsUrlLoader>(
    //     *default_factory_, InterRequestState::FromBrowserContext(context_));
    // ptr->StartRequest(ptr, request, std::move(loader), std::move(client));

  } /* else if (scheme_ == "ipns") {
     auto ptr = std::make_shared<IpnsUrlLoader>(
         InterRequestState::FromBrowserContext(context_), request.url.host(),
         network_context_, *default_factory_);
     ptr->StartHandling(ptr, request, std::move(loader), std::move(client));

   } else {
     NOTREACHED();
   }
      */
}
}  // namespace ipfs
