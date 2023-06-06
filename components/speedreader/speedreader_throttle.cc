/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_throttle.h"

#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/components/speedreader/speedreader_local_url_loader.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_throttle_delegate.h"
#include "brave/components/speedreader/speedreader_url_loader.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace speedreader {

// static
std::unique_ptr<SpeedReaderThrottle>
SpeedReaderThrottle::MaybeCreateThrottleFor(
    SpeedreaderRewriterService* rewriter_service,
    SpeedreaderService* speedreader_service,
    base::WeakPtr<SpeedreaderThrottleDelegate> speedreader_delegate,
    const GURL& url,
    bool check_disabled_sites,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  DCHECK(speedreader_delegate);
  if (!speedreader_delegate->IsPageDistillationAllowed()) {
    return nullptr;
  }

  return std::make_unique<SpeedReaderThrottle>(
      rewriter_service, speedreader_service, speedreader_delegate, task_runner);
}

SpeedReaderThrottle::SpeedReaderThrottle(
    SpeedreaderRewriterService* rewriter_service,
    SpeedreaderService* speedreader_service,
    base::WeakPtr<SpeedreaderThrottleDelegate> speedreader_delegate,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(task_runner),
      rewriter_service_(rewriter_service),
      speedreader_service_(speedreader_service),
      speedreader_delegate_(speedreader_delegate) {}

SpeedReaderThrottle::~SpeedReaderThrottle() = default;

void SpeedReaderThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  if (!speedreader_delegate_ ||
      !speedreader_delegate_->IsPageDistillationAllowed()) {
    // The page was redirected to an ineligible URL. Skip.
    return;
  }

  std::string mime_type;
  if (!response_head || !response_head->headers->GetMimeType(&mime_type) ||
      base::CompareCaseInsensitiveASCII(mime_type, "text/html")) {
    // Skip all non-html documents.
    return;
  }
  VLOG(2) << "Speedreader throttling: " << response_url;

  if (speedreader_delegate_->IsPageContentPresent()) {
    // We've got the content, starting the local source body producer.
    StartSpeedReaderLocalUrlLoader(response_url);
  } else {
    *defer = true;
    // Start the loader which actually performs the distillation.
    StartSpeedReaderUrlLoader(response_url);
  }
}

void SpeedReaderThrottle::StartSpeedReaderLocalUrlLoader(
    const GURL& response_url) {
  mojo::PendingRemote<network::mojom::URLLoader> new_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
  raw_ptr<SpeedReaderLocalURLLoader> speedreader_local_loader = nullptr;

  auto page_content = speedreader_delegate_->TakePageContent();

  std::tie(new_remote, new_receiver, speedreader_local_loader) =
      SpeedReaderLocalURLLoader::CreateLoader(
          AsWeakPtr(), std::move(speedreader_delegate_), task_runner_);

  mojo::PendingRemote<network::mojom::URLLoader> source_loader;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> source_client_receiver;

  delegate_->InterceptResponse(
      std::move(new_remote), std::move(new_receiver), &source_loader,
      &source_client_receiver,
      speedreader_local_loader->GetDestinationConsumerHandle());

  // Drop network.
  source_loader.reset();
  source_client_receiver.reset();

  speedreader_local_loader->Start(std::move(page_content));
}

void SpeedReaderThrottle::StartSpeedReaderUrlLoader(const GURL& response_url) {
  mojo::PendingRemote<network::mojom::URLLoader> new_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
  raw_ptr<SpeedReaderURLLoader> speedreader_loader = nullptr;

  std::tie(new_remote, new_receiver, speedreader_loader) =
      SpeedReaderURLLoader::CreateLoader(
          AsWeakPtr(), std::move(speedreader_delegate_), response_url,
          task_runner_, rewriter_service_, speedreader_service_);
  BodySnifferThrottle::InterceptAndStartLoader(
      std::move(new_remote), std::move(new_receiver), speedreader_loader);
}

}  // namespace speedreader
