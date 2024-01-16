/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
#define BRAVE_RENDERER_BRAVE_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "chrome/renderer/url_loader_throttle_provider_impl.h"

class BraveContentRendererClient;

class BraveURLLoaderThrottleProviderImpl
    : public URLLoaderThrottleProviderImpl {
 public:
  BraveURLLoaderThrottleProviderImpl(
      blink::URLLoaderThrottleProviderType type,
      ChromeContentRendererClient* chrome_content_renderer_client,
      mojo::PendingRemote<safe_browsing::mojom::SafeBrowsing>
          pending_safe_browsing,
#if BUILDFLAG(ENABLE_EXTENSIONS)
      mojo::PendingRemote<safe_browsing::mojom::ExtensionWebRequestReporter>
          pending_extension_web_request_reporter,
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
      scoped_refptr<base::SequencedTaskRunner> main_thread_task_runner,
      base::PassKey<URLLoaderThrottleProviderImpl>);
  ~BraveURLLoaderThrottleProviderImpl() override;

  BraveURLLoaderThrottleProviderImpl(
      const BraveURLLoaderThrottleProviderImpl&) = delete;
  BraveURLLoaderThrottleProviderImpl& operator=(
      const BraveURLLoaderThrottleProviderImpl&) = delete;

  // blink::URLLoaderThrottleProvider implementation.
  blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>> CreateThrottles(
      base::optional_ref<const blink::LocalFrameToken> local_frame_token,
      const network::ResourceRequest& request) override;

 private:
  raw_ptr<BraveContentRendererClient> brave_content_renderer_client_ = nullptr;
};

#endif  // BRAVE_RENDERER_BRAVE_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
