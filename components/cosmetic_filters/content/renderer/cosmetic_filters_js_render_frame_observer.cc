/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/content/renderer/cosmetic_filters_js_render_frame_observer.h"

#include "base/bind.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace cosmetic_filters {

CosmeticFiltersJsRenderFrameObserver::CosmeticFiltersJsRenderFrameObserver(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      worker_isolated_world_id_(isolated_world_id) {}

CosmeticFiltersJsRenderFrameObserver::~CosmeticFiltersJsRenderFrameObserver() {}

void CosmeticFiltersJsRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    base::Optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

void CosmeticFiltersJsRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame()->IsMainFrame() || world_id != worker_isolated_world_id_ ||
      !native_javascript_handle_)
    return;

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void CosmeticFiltersJsRenderFrameObserver::DidCreateNewDocument() {
  // There could be empty and "about:blank" URLs, empty URLs are duplicated
  // with DidCreateDocumentElement, so we just skip them, "about:blank"
  // should fallback to the main frame rules
  if (url_.is_empty())
    return;
  if (url_.spec() == "about:blank") {
    url_ = url::Origin(
        render_frame()->GetWebFrame()->GetSecurityOrigin()).GetURL();
  }
  if (!native_javascript_handle_) {
    native_javascript_handle_.reset(new CosmeticFiltersJSHandler(
        render_frame(), worker_isolated_world_id_));
  }
  native_javascript_handle_->ProcessURL(url_);
}

void CosmeticFiltersJsRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace cosmetic_filters
