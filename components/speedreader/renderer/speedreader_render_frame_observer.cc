// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/renderer/speedreader_render_frame_observer.h"

#include "brave/components/speedreader/renderer/speedreader_js_handler.h"
#include "content/public/renderer/render_frame.h"

namespace speedreader {

SpeedreaderRenderFrameObserver::SpeedreaderRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      isolated_world_id_(isolated_world_id) {}

SpeedreaderRenderFrameObserver::~SpeedreaderRenderFrameObserver() = default;

void SpeedreaderRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame() || !render_frame()->IsMainFrame() ||
      isolated_world_id_ != world_id) {
    return;
  }

  SpeedreaderJSHandler::Install(weak_ptr_factory_.GetWeakPtr(), context);
}

void SpeedreaderRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace speedreader
