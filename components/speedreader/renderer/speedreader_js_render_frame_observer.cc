/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/renderer/speedreader_js_render_frame_observer.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace speedreader {

SpeedreaderJsRenderFrameObserver::SpeedreaderJsRenderFrameObserver(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : RenderFrameObserver(render_frame), isolated_world_id_(isolated_world_id) {
  render_frame->GetRemoteAssociatedInterfaces()->GetInterface(
      &speedreader_result_remote_);
}

SpeedreaderJsRenderFrameObserver::~SpeedreaderJsRenderFrameObserver() = default;

void SpeedreaderJsRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  DCHECK(speedreader_result_remote_);
  if (world_id != isolated_world_id_ || !render_frame()->IsMainFrame())
    return;

  speedreader_result_remote_->GetPageDistilled(
      base::BindOnce(&SpeedreaderJsRenderFrameObserver::OnPageDistillResult,
                     base::Unretained(this)));
}

void SpeedreaderJsRenderFrameObserver::OnPageDistillResult(bool is_distilled) {
  if (is_distilled) {
    speedreader_js_handler_ = std::make_unique<SpeedreaderJsHandler>(
        render_frame(), isolated_world_id_);
  }
}

void SpeedreaderJsRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace speedreader
