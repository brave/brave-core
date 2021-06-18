// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/renderer/brave_search_render_frame_observer.h"

#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "brave/components/brave_search/renderer/brave_search_default_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/gurl.h"

namespace brave_search {

BraveSearchRenderFrameObserver::BraveSearchRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {}

BraveSearchRenderFrameObserver::~BraveSearchRenderFrameObserver() {}

void BraveSearchRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame()->IsMainFrame() || world_id_ != world_id)
    return;

  GURL url =
      url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin()).GetURL();

  if (!IsAllowedHost(url))
    return;

  if (!native_javascript_handle_) {
    native_javascript_handle_.reset(
        new BraveSearchDefaultJSHandler(render_frame()));
  } else {
    native_javascript_handle_->ResetRemote(render_frame());
  }

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void BraveSearchRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_search
