/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "services/network/public/cpp/self_deleting_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

#include <string>

namespace content {
class BrowserContext;
}

namespace network::mojom {
class NetworkContext;
}

namespace ipfs {

class IpfsTrustlessClientUrlLoaderFactory
    : public network::SelfDeletingURLLoaderFactory {
 public:
  static void Create(
      std::map<std::string,
               mojo::PendingRemote<network::mojom::URLLoaderFactory>>* in_out,
      content::BrowserContext*,
      URLLoaderFactory*,
      network::mojom::NetworkContext*);

private:
  IpfsTrustlessClientUrlLoaderFactory(
      std::string scheme,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver,
      content::BrowserContext* context,
      URLLoaderFactory* default_factory,
      network::mojom::NetworkContext* net_ctxt);
  ~IpfsTrustlessClientUrlLoaderFactory() override;

  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      network::ResourceRequest const& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      net::MutableNetworkTrafficAnnotationTag const& traffic_annotation)
      override;

  std::string scheme_;
  raw_ptr<content::BrowserContext> context_;
  raw_ptr<network::mojom::URLLoaderFactory> default_factory_;
  raw_ptr<network::mojom::NetworkContext> network_context_;    
};

}  // namespace ipfs
