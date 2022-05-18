/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_talk/renderer/brave_talk_render_frame_observer.h"

#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "brave/components/brave_talk/common/brave_talk_utils.h"
#include "brave/components/brave_talk/renderer/brave_talk_frame_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/gurl.h"

namespace brave_talk {

BraveTalkRenderFrameObserver::BraveTalkRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {}

BraveTalkRenderFrameObserver::~BraveTalkRenderFrameObserver() {}

void BraveTalkRenderFrameObserver::DidCreateScriptContext(
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
        new BraveTalkFrameJSHandler(render_frame()));
  } else {
    native_javascript_handle_->ResetRemote(render_frame());
  }

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void BraveTalkRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_search
