// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_ads/renderer/brave_ads_render_frame_observer.h"

#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "brave/components/brave_ads/renderer/brave_ads_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr auto kVettedHosts = base::MakeFixedFlatSet<base::StringPiece>(
    {"talk.brave.com", "beta.talk.brave.com", "talk.bravesoftware.com",
     "beta.talk.bravesoftware.com", "talk.brave.software",
     "beta.talk.brave.software", "dev.talk.brave.software"});

bool IsAllowedHost(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }
  const std::string host = url.host();
  return base::Contains(kVettedHosts, host);
}

}  // namespace

BraveAdsRenderFrameObserver::BraveAdsRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {
  native_javascript_handle_ = std::make_unique<BraveAdsJSHandler>(render_frame);
}

BraveAdsRenderFrameObserver::~BraveAdsRenderFrameObserver() {}

void BraveAdsRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame()->IsMainFrame() || world_id_ != world_id)
    return;

  const GURL url =
      url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin()).GetURL();

  if (!IsAllowedHost(url))
    return;

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void BraveAdsRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_ads
