/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/renderer/psst_render_frame_observer.h"

#include "brave/components/psst/renderer/psst_js_handler.h"
#include "content/public/renderer/render_frame.h"

namespace psst {

PsstRenderFrameObserver::PsstRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      isolated_world_id_(isolated_world_id) {}

PsstRenderFrameObserver::~PsstRenderFrameObserver() = default;

void PsstRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  // Subframes are not filtered out: the policy script can navigate into and
  // operate on frames, and their logs are attributed per-frame.
  if (!render_frame() || isolated_world_id_ != world_id) {
    return;
  }

  PsstJSHandler::Install(render_frame(), context);
}

void PsstRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace psst
