/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_URL_LOADER_THROTTLE_PROVIDER_H_
#define BRAVE_RENDERER_BRAVE_URL_LOADER_THROTTLE_PROVIDER_H_

#include <memory>

#include "third_party/blink/public/platform/url_loader_throttle_provider.h"

class ChromeContentRendererClient;

namespace blink {
class ThreadSafeBrowserInterfaceBrokerProxy;
}

class BraveURLLoaderThrottleProvider : public blink::URLLoaderThrottleProvider {
 public:
  BraveURLLoaderThrottleProvider(
      blink::ThreadSafeBrowserInterfaceBrokerProxy* broker,
      blink::URLLoaderThrottleProviderType type,
      ChromeContentRendererClient* chrome_content_renderer_client);

  BraveURLLoaderThrottleProvider(const BraveURLLoaderThrottleProvider& other) =
      delete;
  BraveURLLoaderThrottleProvider& operator=(
      const BraveURLLoaderThrottleProvider&) = delete;

  ~BraveURLLoaderThrottleProvider() override;

  // blink::URLLoaderThrottleProvider implementation.
  std::unique_ptr<blink::URLLoaderThrottleProvider> Clone() override;
  blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>> CreateThrottles(
      int render_frame_id,
      const blink::WebURLRequest& request) override;
  void SetOnline(bool is_online) override;

 private:
  BraveURLLoaderThrottleProvider();

  blink::URLLoaderThrottleProviderType provider_type_ =
      blink::URLLoaderThrottleProviderType::kFrame;

  std::unique_ptr<blink::URLLoaderThrottleProvider>
      url_loader_throttle_provider_impl_;
};

#endif  // BRAVE_RENDERER_BRAVE_URL_LOADER_THROTTLE_PROVIDER_H_
