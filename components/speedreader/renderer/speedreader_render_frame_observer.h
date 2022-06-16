// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "v8/include/v8.h"

namespace speedreader {

class SpeedreaderRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit SpeedreaderRenderFrameObserver(content::RenderFrame* render_frame,
                                          int32_t world_id);
  SpeedreaderRenderFrameObserver(const SpeedreaderRenderFrameObserver&) =
      delete;
  SpeedreaderRenderFrameObserver& operator=(
      const SpeedreaderRenderFrameObserver&) = delete;
  ~SpeedreaderRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // Handle to "handler" JavaScript object functionality.
  std::unique_ptr<class SpeedreaderJSHandler> native_javascript_handle_;
  int32_t world_id_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_RENDER_FRAME_OBSERVER_H_
