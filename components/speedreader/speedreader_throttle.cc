/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_throttle.h"

#include <utility>

#include "brave/components/speedreader/speedreader_loader.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace speedreader {

SpeedReaderThrottle::SpeedReaderThrottle(
    SpeedreaderWhitelist* whitelist,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : speedreader_whitelist_(whitelist), task_runner_(std::move(task_runner)) {}

SpeedReaderThrottle::~SpeedReaderThrottle() = default;

void SpeedReaderThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  VLOG(2) << "Speedreader throttling: " << response_url;
  // Pause the response until Speedreader has done its job.
  *defer = true;

  mojo::PendingRemote<network::mojom::URLLoader> new_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
  mojo::PendingRemote<network::mojom::URLLoader> source_loader;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> source_client_receiver;
  SpeedReaderURLLoader* speedreader_loader;
  std::tie(new_remote, new_receiver, speedreader_loader) =
      SpeedReaderURLLoader::CreateLoader(weak_factory_.GetWeakPtr(),
                                         response_url, task_runner_,
                                         speedreader_whitelist_);
  delegate_->InterceptResponse(std::move(new_remote), std::move(new_receiver),
                               &source_loader, &source_client_receiver);
  speedreader_loader->Start(std::move(source_loader),
                            std::move(source_client_receiver));
}

void SpeedReaderThrottle::Resume() {
  delegate_->Resume();
}

}  // namespace speedreader
