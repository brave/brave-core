// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_RENDERER_BRAVE_ADS_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_RENDERER_BRAVE_ADS_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "content/public/renderer/render_frame_observer.h"
#include "v8/include/v8.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace brave_ads {

class BraveAdsJSHandler;

class BraveAdsRenderFrameObserver : public content::RenderFrameObserver {
 public:
  BraveAdsRenderFrameObserver(content::RenderFrame* render_frame,
                              int32_t world_id);
  BraveAdsRenderFrameObserver(const BraveAdsRenderFrameObserver&) = delete;
  BraveAdsRenderFrameObserver& operator=(const BraveAdsRenderFrameObserver&) =
      delete;
  ~BraveAdsRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // Handle to "handler" JavaScript object functionality.
  std::unique_ptr<BraveAdsJSHandler> native_javascript_handle_;
  int32_t world_id_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_RENDERER_BRAVE_ADS_RENDER_FRAME_OBSERVER_H_
