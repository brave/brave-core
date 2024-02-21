// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/renderer/ai_chat_resource_sniffer_throttle.h"
#include "brave/components/ai_chat/renderer/ai_chat_resource_sniffer_url_loader.h"
#include "brave/components/ai_chat/renderer/yt_util.h"

namespace ai_chat {

namespace {
constexpr char kYouTubePlayerAPIPath[] = "/youtubei/v1/player";
}

std::unique_ptr<AIChatResourceSnifferThrottle>
AIChatResourceSnifferThrottle::MaybeCreateThrottleFor(
    base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate,
    const GURL& url,
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  DCHECK(delegate);

  if (url.SchemeIsHTTPOrHTTPS() && base::Contains(kYouTubeHosts, url.host()) &&
      base::EqualsCaseInsensitiveASCII(url.path(), kYouTubePlayerAPIPath)) {
    VLOG(1) << __func__ << " Creating throttle for url: " << url.spec();
    return std::make_unique<AIChatResourceSnifferThrottle>(task_runner,
                                                           delegate);
  }
  return nullptr;
}

AIChatResourceSnifferThrottle::AIChatResourceSnifferThrottle(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    base::WeakPtr<AIChatResourceSnifferThrottleDelegate> delegate)
    : task_runner_(task_runner), delegate_(delegate) {}

AIChatResourceSnifferThrottle::~AIChatResourceSnifferThrottle() = default;

void AIChatResourceSnifferThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  mojo::PendingRemote<network::mojom::URLLoader> new_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
  raw_ptr<AIChatResourceSnifferURLLoader> sniffer_loader = nullptr;

  std::tie(new_remote, new_receiver, sniffer_loader) =
      AIChatResourceSnifferURLLoader::CreateLoader(
          AsWeakPtr(), std::move(delegate_), task_runner_, response_url);
  BodySnifferThrottle::InterceptAndStartLoader(
      std::move(new_remote), std::move(new_receiver), sniffer_loader);
}

}  // namespace ai_chat
