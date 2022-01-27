// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/renderer/skus_render_frame_observer.h"

#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/skus/common/features.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace skus {

SkusRenderFrameObserver::SkusRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {}

SkusRenderFrameObserver::~SkusRenderFrameObserver() {}

void SkusRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame()->IsMainFrame() || world_id_ != world_id)
    return;

  if (!IsAllowed())
    return;

  if (!native_javascript_handle_) {
    native_javascript_handle_ = std::make_unique<SkusJSHandler>(render_frame());
  } else {
    native_javascript_handle_->ResetRemote(render_frame());
  }

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

bool SkusRenderFrameObserver::IsAllowed() {
  DCHECK(base::FeatureList::IsEnabled(skus::features::kSkusFeature));
  // NOTE: please open a security review when appending to this list.
  static base::NoDestructor<std::vector<blink::WebSecurityOrigin>> safe_origins{
      {{blink::WebSecurityOrigin::Create(GURL("https://account.brave.com"))},
       {blink::WebSecurityOrigin::Create(
           GURL("https://account.bravesoftware.com"))},
       {blink::WebSecurityOrigin::Create(
           GURL("https://account.brave.software"))}}};

  const blink::WebSecurityOrigin& visited_origin =
      render_frame()->GetWebFrame()->GetSecurityOrigin();
  for (const blink::WebSecurityOrigin& safe_origin : *safe_origins) {
    if (safe_origin.IsSameOriginWith(visited_origin)) {
      return true;
    }
  }

  return false;
}

void SkusRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace skus
