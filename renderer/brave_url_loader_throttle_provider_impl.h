/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
#define BRAVE_RENDERER_BRAVE_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

#include <memory>

#include "base/functional/callback.h"
#include "chrome/renderer/url_loader_throttle_provider_impl.h"

class BraveURLLoaderThrottleProviderImpl
    : public URLLoaderThrottleProviderImpl {
 public:
  BraveURLLoaderThrottleProviderImpl(
      blink::ThreadSafeBrowserInterfaceBrokerProxy* broker,
      blink::URLLoaderThrottleProviderType type,
      ChromeContentRendererClient* chrome_content_renderer_client,
      base::RepeatingCallback<bool()> read_is_tor_cb);
  ~BraveURLLoaderThrottleProviderImpl() override;

  BraveURLLoaderThrottleProviderImpl(
      const BraveURLLoaderThrottleProviderImpl&) = delete;
  BraveURLLoaderThrottleProviderImpl& operator=(
      const BraveURLLoaderThrottleProviderImpl&) = delete;

  // blink::URLLoaderThrottleProvider implementation.
  blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>> CreateThrottles(
      int render_frame_id,
      const blink::WebURLRequest& request) override;

 private:
  base::RepeatingCallback<bool()> read_is_tor_cb_;
};

#endif  // BRAVE_RENDERER_BRAVE_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
