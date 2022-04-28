/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_url_loader_throttle_provider.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "brave/components/brave_ads/renderer/search_result_ad_renderer_throttle.h"
#include "chrome/renderer/chrome_content_renderer_client.h"
#include "chrome/renderer/url_loader_throttle_provider_impl.h"
#include "third_party/blink/public/common/thread_safe_browser_interface_broker_proxy.h"

BraveURLLoaderThrottleProvider::BraveURLLoaderThrottleProvider(
    blink::ThreadSafeBrowserInterfaceBrokerProxy* broker,
    blink::URLLoaderThrottleProviderType type,
    ChromeContentRendererClient* chrome_content_renderer_client)
    : provider_type_(type),
      url_loader_throttle_provider_impl_(
          std::make_unique<URLLoaderThrottleProviderImpl>(
              broker,
              type,
              chrome_content_renderer_client)) {}

BraveURLLoaderThrottleProvider::~BraveURLLoaderThrottleProvider() {}

BraveURLLoaderThrottleProvider::BraveURLLoaderThrottleProvider() {}

std::unique_ptr<blink::URLLoaderThrottleProvider>
BraveURLLoaderThrottleProvider::Clone() {
  auto throttle_provider_clone =
      base::WrapUnique(new BraveURLLoaderThrottleProvider());

  throttle_provider_clone->provider_type_ = provider_type_;
  throttle_provider_clone->url_loader_throttle_provider_impl_ =
      url_loader_throttle_provider_impl_->Clone();
  return throttle_provider_clone;
}

blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>>
BraveURLLoaderThrottleProvider::CreateThrottles(
    int render_frame_id,
    const blink::WebURLRequest& request) {
  blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>> throttles =
      url_loader_throttle_provider_impl_->CreateThrottles(render_frame_id,
                                                          request);

  if (provider_type_ == blink::URLLoaderThrottleProviderType::kFrame) {
    auto search_result_ad_throttle =
        brave_ads::SearchResultAdRendererThrottle::MaybeCreateThrottle(
            render_frame_id, request);
    if (search_result_ad_throttle) {
      throttles.emplace_back(std::move(search_result_ad_throttle));
    }
  }

  return throttles;
}

void BraveURLLoaderThrottleProvider::SetOnline(bool is_online) {
  url_loader_throttle_provider_impl_->SetOnline(is_online);
}
