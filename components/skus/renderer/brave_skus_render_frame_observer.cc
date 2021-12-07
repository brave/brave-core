// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/renderer/brave_skus_render_frame_observer.h"

#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/skus/common/features.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace brave_rewards {

BraveSkusRenderFrameObserver::BraveSkusRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {}

BraveSkusRenderFrameObserver::~BraveSkusRenderFrameObserver() {}

void BraveSkusRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    absl::optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

void BraveSkusRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame()->IsMainFrame() || world_id_ != world_id)
    return;

  // There could be empty, invalid and "about:blank" URLs,
  // they should fallback to the main frame rules
  if (url_.is_empty() || !url_.is_valid() || url_.spec() == "about:blank")
    url_ = url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
               .GetURL();

  if (!IsSkusSdkAllowed())
    return;

  if (!native_javascript_handle_) {
    native_javascript_handle_.reset(new BraveSkusJSHandler(render_frame()));
  } else {
    native_javascript_handle_->ResetRemote(render_frame());
  }

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

bool BraveSkusRenderFrameObserver::IsSkusSdkAllowed() {
  return base::FeatureList::IsEnabled(skus::features::kSdkFeature) &&
         (url_.host() == "account.brave.com" ||
          url_.host() == "account.bravesoftware.com" ||
          url_.host() == "account.brave.software");
}

void BraveSkusRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_rewards
