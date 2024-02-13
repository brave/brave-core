// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/renderer/brave_search_render_frame_observer.h"

#include <memory>
#include <optional>

#include "brave/components/brave_search/common/brave_search_utils.h"
#include "brave/components/brave_search/renderer/brave_search_default_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "net/base/url_util.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/gurl.h"

namespace brave_search {

BraveSearchRenderFrameObserver::BraveSearchRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {}

BraveSearchRenderFrameObserver::~BraveSearchRenderFrameObserver() = default;

void BraveSearchRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame()->IsMainFrame() || world_id_ != world_id)
    return;

  GURL origin =
      url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin()).GetURL();

  if (!IsAllowedHost(origin))
    return;

  bool can_always_set_default = false;
  for (net::QueryIterator it(url_); !it.IsAtEnd(); it.Advance()) {
    if (it.GetKey() == "action" && it.GetValue() == "makeDefault") {
      can_always_set_default = true;
      break;
    }
  }

  if (!native_javascript_handle_) {
    native_javascript_handle_ = std::make_unique<BraveSearchDefaultJSHandler>(
        render_frame(), can_always_set_default);
  } else {
    native_javascript_handle_->ResetRemote(render_frame());
  }

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void BraveSearchRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    std::optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

void BraveSearchRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_search
