/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"
#include "brave/renderer/brave_url_loader_throttle_provider.h"
#include "chrome/renderer/url_loader_throttle_provider_impl.h"

#define URLLoaderThrottleProviderImpl BraveURLLoaderThrottleProvider

// We need to do this here rather than in |BraveContentRendererClient| because
// it needs access to the registry on ChromeRenderFrameObserver.
#define BRAVE_RENDER_FRAME_CREATED \
  new feed::RssLinkReader(render_frame, registry);

#include "src/chrome/renderer/chrome_content_renderer_client.cc"

#undef BRAVE_RENDER_FRAME_CREATED
#undef URLLoaderThrottleProviderImpl
