/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "brave/components/speedreader/common/speedreader_result.mojom.h"
#include "brave/components/speedreader/renderer/speedreader_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "v8/include/v8.h"

namespace speedreader {

class SpeedreaderJsRenderFrameObserver : public content::RenderFrameObserver {
 public:
  SpeedreaderJsRenderFrameObserver(content::RenderFrame* render_frame,
                                   const int32_t isolated_world_id);
  ~SpeedreaderJsRenderFrameObserver() override;

  // content::RenderFrameObserver:
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  // content::RenderFrameObserver:
  void OnDestruct() override;

  void OnPageDistillResult(bool is_distilled);

  int32_t isolated_world_id_;
  std::unique_ptr<SpeedreaderJsHandler> speedreader_js_handler_;
  mojo::AssociatedRemote<mojom::SpeedreaderResult> speedreader_result_remote_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_RENDER_FRAME_OBSERVER_H_
