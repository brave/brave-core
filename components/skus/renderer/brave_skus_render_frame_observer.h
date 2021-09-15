// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_RENDERER_BRAVE_SKUS_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_SKUS_RENDERER_BRAVE_SKUS_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "brave/components/skus/renderer/brave_skus_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "v8/include/v8.h"

namespace brave_rewards {

class BraveSkusRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit BraveSkusRenderFrameObserver(content::RenderFrame* render_frame,
                                          int32_t world_id);
  BraveSkusRenderFrameObserver(const BraveSkusRenderFrameObserver&) =
      delete;
  BraveSkusRenderFrameObserver& operator=(
      const BraveSkusRenderFrameObserver&) = delete;
  ~BraveSkusRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // Handle to "handler" JavaScript object functionality.
  std::unique_ptr<BraveSkusJSHandler> native_javascript_handle_;
  int32_t world_id_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_RENDERER_BRAVE_SKUS_RENDER_FRAME_OBSERVER_H_
