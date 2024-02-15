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

// static
std::unique_ptr<blink::URLLoaderThrottleProvider>
BraveURLLoaderThrottleProviderImpl::Create(
    blink::URLLoaderThrottleProviderType type,
    ChromeContentRendererClient* chrome_content_renderer_client,
    blink::ThreadSafeBrowserInterfaceBrokerProxy* broker) {
  mojo::PendingRemote<safe_browsing::mojom::SafeBrowsing> pending_safe_browsing;
  broker->GetInterface(pending_safe_browsing.InitWithNewPipeAndPassReceiver());
#if BUILDFLAG(ENABLE_EXTENSIONS)
  mojo::PendingRemote<safe_browsing::mojom::ExtensionWebRequestReporter>
      pending_extension_web_request_reporter;
  broker->GetInterface(
      pending_extension_web_request_reporter.InitWithNewPipeAndPassReceiver());
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

  return std::make_unique<BraveURLLoaderThrottleProviderImpl>(
      type, chrome_content_renderer_client, std::move(pending_safe_browsing),
#if BUILDFLAG(ENABLE_EXTENSIONS)
      std::move(pending_extension_web_request_reporter),
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
      /*main_thread_task_runner=*/
      content::RenderThread::IsMainThread()
          ? base::SequencedTaskRunner::GetCurrentDefault()
          : nullptr);
}
BraveURLLoaderThrottleProviderImpl::BraveURLLoaderThrottleProviderImpl(
    blink::URLLoaderThrottleProviderType type,
    ChromeContentRendererClient* chrome_content_renderer_client,
    mojo::PendingRemote<safe_browsing::mojom::SafeBrowsing>
        pending_safe_browsing,
#if BUILDFLAG(ENABLE_EXTENSIONS)
    mojo::PendingRemote<safe_browsing::mojom::ExtensionWebRequestReporter>
        pending_extension_web_request_reporter,
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
    scoped_refptr<base::SequencedTaskRunner> main_thread_task_runner)
    : URLLoaderThrottleProviderImpl(
          type,
          chrome_content_renderer_client,
          std::move(pending_safe_browsing),
#if BUILDFLAG(ENABLE_EXTENSIONS)
          std::move(pending_extension_web_request_reporter),
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
          std::move(main_thread_task_runner),
          GetPassKey()),
      brave_content_renderer_client_(static_cast<BraveContentRendererClient*>(
          chrome_content_renderer_client)) {
}

BraveURLLoaderThrottleProviderImpl::~BraveURLLoaderThrottleProviderImpl() =
    default;

blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>>
BraveURLLoaderThrottleProviderImpl::CreateThrottles(
    base::optional_ref<const blink::LocalFrameToken> local_frame_token,
    const network::ResourceRequest& request) {
  auto throttles = URLLoaderThrottleProviderImpl::CreateThrottles(
      local_frame_token, request);
#if BUILDFLAG(ENABLE_TOR)
  if (auto onion_domain_throttle =
          tor::OnionDomainThrottle::MaybeCreateThrottle(
              brave_content_renderer_client_->IsOnionAllowed())) {
    throttles.emplace_back(std::move(onion_domain_throttle));
  }
#endif
  return throttles;
}
