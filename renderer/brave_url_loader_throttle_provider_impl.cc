/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_url_loader_throttle_provider_impl.h"

#include <utility>

#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/renderer/brave_content_renderer_client.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/renderer/onion_domain_throttle.h"
#endif

BraveURLLoaderThrottleProviderImpl::BraveURLLoaderThrottleProviderImpl(
    blink::ThreadSafeBrowserInterfaceBrokerProxy* broker,
    blink::URLLoaderThrottleProviderType type,
    ChromeContentRendererClient* chrome_content_renderer_client)
    : URLLoaderThrottleProviderImpl(broker,
                                    type,
                                    chrome_content_renderer_client),
      brave_content_renderer_client_(static_cast<BraveContentRendererClient*>(
          chrome_content_renderer_client)) {}

BraveURLLoaderThrottleProviderImpl::~BraveURLLoaderThrottleProviderImpl() =
    default;

blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>>
BraveURLLoaderThrottleProviderImpl::CreateThrottles(
    int render_frame_id,
    const blink::WebURLRequest& request) {
  auto throttles =
      URLLoaderThrottleProviderImpl::CreateThrottles(render_frame_id, request);
#if BUILDFLAG(ENABLE_TOR)
  if (auto onion_domain_throttle =
          tor::OnionDomainThrottle::MaybeCreateThrottle(
              brave_content_renderer_client_->IsTorProcess())) {
    throttles.emplace_back(std::move(onion_domain_throttle));
  }
#endif
  return throttles;
}
