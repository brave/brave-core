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

void SpeedreaderRenderFrameObserver::DidClearWindowObject() {
  if (!render_frame()->IsMainFrame()) {
    return;
  }
  SpeedreaderJSHandler::Install(weak_ptr_factory_.GetWeakPtr(),
                                isolated_world_id_);
}

void SpeedreaderRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace speedreader
