/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_throttle.h"

#include <utility>

#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "brave/components/de_amp/browser/de_amp_service.h"
#include "brave/components/de_amp/browser/de_amp_url_loader.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace de_amp {

// static
std::unique_ptr<DeAmpThrottle> DeAmpThrottle::MaybeCreateThrottleFor(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    DeAmpService* service,
    content::WebContents* contents) {
  if (!service->IsEnabled()) {
    return nullptr;
  }
  return std::make_unique<DeAmpThrottle>(task_runner, service, contents);
}

DeAmpThrottle::DeAmpThrottle(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    DeAmpService* service,
    content::WebContents* contents)
    : task_runner_(task_runner), service_(service), contents_(contents) {}

DeAmpThrottle::~DeAmpThrottle() = default;

void DeAmpThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  VLOG(2) << "deamp throttling: " << response_url;

  mojo::PendingRemote<network::mojom::URLLoader> new_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
  mojo::PendingRemote<network::mojom::URLLoader> source_loader;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> source_client_receiver;
  DeAmpURLLoader* de_amp_loader;
  std::tie(new_remote, new_receiver, de_amp_loader) =
      DeAmpURLLoader::CreateLoader(weak_factory_.GetWeakPtr(), response_url,
                                   task_runner_, service_, contents_);
  BodySnifferThrottle::InterceptAndStartLoader(
      std::move(source_loader), std::move(source_client_receiver),
      std::move(new_remote), std::move(new_receiver), de_amp_loader, defer);
}

}  // namespace de_amp
