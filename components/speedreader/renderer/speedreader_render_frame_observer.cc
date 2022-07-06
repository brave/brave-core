// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/renderer/speedreader_render_frame_observer.h"

#include "brave/components/speedreader/common/constants.h"
#include "brave/components/speedreader/common/url_readable_hints.h"
#include "brave/components/speedreader/renderer/speedreader_js_handler.h"
#include "content/public/renderer/render_frame.h"

namespace speedreader {

SpeedreaderRenderFrameObserver::SpeedreaderRenderFrameObserver(
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

SpeedreaderRenderFrameObserver::~SpeedreaderRenderFrameObserver() = default;

void SpeedreaderRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    absl::optional<blink::WebNavigationType> navigation_type) {
  is_speedreadable_url_ = IsURLLooksReadable(url);
}

void SpeedreaderRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!is_speedreadable_url_ || !render_frame()->IsMainFrame() ||
      world_id != kIsolatedWorldId) {
    return;
  }

  if (!native_javascript_handle_) {
    native_javascript_handle_.reset(new SpeedreaderJSHandler(render_frame()));
  } else {
    native_javascript_handle_->ResetRemote(render_frame());
  }

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void SpeedreaderRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace speedreader
