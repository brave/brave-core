// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BNES_BNS_SCHEME_HANDLER_H_
#define BRAVE_BNES_BNS_SCHEME_HANDLER_H_

#include <memory>
#include <string_view>

#include "base/component_export.h"
#include "base/memory/self_deleting.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/self_deleting_url_loader_factory.h"

namespace network {
class URLLoaderFactory;
}

namespace bnes {

// BnesURLLoaderFactory implements the native `bnes://` handler by subclassing
// network::SelfDeletingURLLoaderFactory. It is injected via the
// ContentBrowserClient::WillCreateURLLoaderFactory() hook (upstream touch point
// #2).
//
// Lifecycle: Created on the UI thread for a navigation request. The factory
// deletes itself after the loader completes.
class BnesURLLoaderFactory : public network::SelfDeletingURLLoaderFactory {
 public:
  // Returns a mojo::PendingRemote to a newly constructed BnesURLLoaderFactory.
  // The factory is self-owned - it will delete itself once there are no more
  // receivers (including the receiver associated with the returned
  // mojo::PendingRemote and the receivers bound by the Clone method).
  static mojo::PendingRemote<network::mojom::URLLoaderFactory> Create();

  BnesURLLoaderFactory(
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver,
      base::SelfDeletingPassKey key);

  BnesURLLoaderFactory(const BnesURLLoaderFactory&) = delete;
  BnesURLLoaderFactory& operator=(const BnesURLLoaderFactory&) = delete;

 private:
  // network::mojom::URLLoaderFactory:
  ~BnesURLLoaderFactory() override;
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;

  void StartAsyncResolve(
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client);
};

}  // namespace bnes

#endif  // BRAVE_BNES_BNS_SCHEME_HANDLER_H_
