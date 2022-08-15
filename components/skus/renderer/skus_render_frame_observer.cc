// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/renderer/skus_render_frame_observer.h"

#include <string>
#include <vector>

#include "base/feature_list.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/skus/renderer/skus_utils.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace skus {

SkusRenderFrameObserver::SkusRenderFrameObserver(
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

SkusRenderFrameObserver::~SkusRenderFrameObserver() = default;

void SkusRenderFrameObserver::DidClearWindowObject() {
  if (!IsAllowed())
    return;

  SkusJSHandler::Install(render_frame());
}

bool SkusRenderFrameObserver::IsAllowed() {
  DCHECK(base::FeatureList::IsEnabled(skus::features::kSkusFeature));

  return skus::IsSafeOrigin(render_frame()->GetWebFrame()->GetSecurityOrigin());
}

void SkusRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace skus
