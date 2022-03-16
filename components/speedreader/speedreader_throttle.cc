/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_throttle.h"

#include <utility>

#include "brave/components/speedreader/speedreader_result_delegate.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_url_loader.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace speedreader {

// static
std::unique_ptr<SpeedReaderThrottle>
SpeedReaderThrottle::MaybeCreateThrottleFor(
    SpeedreaderRewriterService* rewriter_service,
    HostContentSettingsMap* content_settings,
    base::WeakPtr<SpeedreaderResultDelegate> result_delegate,
    const GURL& url,
    bool check_disabled_sites,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  if (check_disabled_sites && !IsEnabledForSite(content_settings, url))
    return nullptr;

  return std::make_unique<SpeedReaderThrottle>(rewriter_service,
                                               result_delegate, task_runner);
}

SpeedReaderThrottle::SpeedReaderThrottle(
    SpeedreaderRewriterService* rewriter_service,
    base::WeakPtr<SpeedreaderResultDelegate> result_delegate,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(task_runner),
      rewriter_service_(rewriter_service),
      result_delegate_(result_delegate) {}

SpeedReaderThrottle::~SpeedReaderThrottle() = default;

void SpeedReaderThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  VLOG(2) << "Speedreader throttling: " << response_url;
  *defer = true;

  mojo::PendingRemote<network::mojom::URLLoader> new_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
  mojo::PendingRemote<network::mojom::URLLoader> source_loader;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> source_client_receiver;
  raw_ptr<SpeedReaderURLLoader> speedreader_loader = nullptr;
  mojo::ScopedDataPipeConsumerHandle body;
  std::tie(new_remote, new_receiver, speedreader_loader) =
      SpeedReaderURLLoader::CreateLoader(AsWeakPtr(), result_delegate_,
                                         response_url, task_runner_,
                                         rewriter_service_);
  BodySnifferThrottle::InterceptAndStartLoader(
      std::move(source_loader), std::move(source_client_receiver),
      std::move(new_remote), std::move(new_receiver), speedreader_loader);
}

}  // namespace speedreader
