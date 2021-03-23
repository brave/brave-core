/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/renderer/brave_search_render_frame_observer.h"

#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace {

static base::NoDestructor<std::vector<std::string>> g_vetted_hosts(
    {"search.brave.com", "search-dev.brave.com"});

bool IsVettedHost(const GURL& url) {
  std::string host = url.host();
  for (size_t i = 0; i < g_vetted_hosts->size(); i++) {
    if ((*g_vetted_hosts)[i] == host)
      return true;
  }

  return false;
}

}  // namespace

namespace brave_search {

BraveSearchRenderFrameObserver::BraveSearchRenderFrameObserver(
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {
  native_javascript_handle_.reset(new BraveSearchJSHandler(render_frame));
}

BraveSearchRenderFrameObserver::~BraveSearchRenderFrameObserver() {}

void BraveSearchRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    base::Optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

void BraveSearchRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  // There could be empty, invalid and "about:blank" URLs,
  // they should fallback to the main frame rules
  if (url_.is_empty() || !url_.is_valid() || url_.spec() == "about:blank")
    url_ = url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
               .GetURL();

  if (!url_.SchemeIsHTTPOrHTTPS() || !native_javascript_handle_ ||
      !IsVettedHost(url_))
    return;

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void BraveSearchRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_search
