/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_throttle.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/components/de_amp/browser/de_amp_url_loader.h"
#include "brave/components/de_amp/common/features.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "content/public/browser/web_contents.h"

namespace de_amp {

  // static
std::unique_ptr<DeAmpThrottle> DeAmpThrottle::MaybeCreateThrottleFor(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    content::WebContents* contents
    ) {
  if (!base::FeatureList::IsEnabled(de_amp::features::kBraveDeAMP)) {
    return nullptr;
  }
  return std::make_unique<DeAmpThrottle>(task_runner, contents);
}

DeAmpThrottle::DeAmpThrottle(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    content::WebContents* contents)
    : task_runner_(std::move(task_runner)), contents_(contents) {}

DeAmpThrottle::~DeAmpThrottle() = default;

void DeAmpThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  VLOG(2) << "deamp throttling: " << response_url;
  // Pause the response until De-Amping is done.
  *defer = true;

  mojo::PendingRemote<network::mojom::URLLoader> new_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
  mojo::PendingRemote<network::mojom::URLLoader> source_loader;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> source_client_receiver;
  DeAmpURLLoader* de_amp_loader;
  std::tie(new_remote, new_receiver, de_amp_loader) =
      DeAmpURLLoader::CreateLoader(weak_factory_.GetWeakPtr(),
                                         response_url,
                                         task_runner_,
                                         contents_);
  delegate_->InterceptResponse(std::move(new_remote), std::move(new_receiver),
                               &source_loader, &source_client_receiver);
  de_amp_loader->Start(std::move(source_loader),
                            std::move(source_client_receiver));
}

void DeAmpThrottle::Resume() {
  delegate_->Resume();
}

}  // namespace de_amp
