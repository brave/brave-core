/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/sniffer/sniffer_throttle.h"
#include "brave/components/sniffer/sniffer_url_loader.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace sniffer {

SnifferThrottle::SnifferThrottle() = default;

SnifferThrottle::~SnifferThrottle() = default;

void SnifferThrottle::InterceptAndStartLoader(
    mojo::PendingRemote<network::mojom::URLLoader> source_loader,
    mojo::PendingReceiver<network::mojom::URLLoaderClient>
        source_client_receiver,
    mojo::PendingRemote<network::mojom::URLLoader> new_remote,
    mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver,
    SnifferURLLoader* loader,
    bool* defer) {
  // Pause the response until loader has done its job.
  *defer = true;

  mojo::ScopedDataPipeConsumerHandle body;
  delegate_->InterceptResponse(std::move(new_remote), std::move(new_receiver),
                               &source_loader, &source_client_receiver, &body);
  loader->Start(std::move(source_loader), std::move(source_client_receiver),
                std::move(body));
}

void SnifferThrottle::Resume() {
  delegate_->Resume();
}

}  // namespace sniffer
