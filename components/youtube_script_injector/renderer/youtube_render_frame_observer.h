/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_RENDERER_YOUTUBE_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_RENDERER_YOUTUBE_RENDER_FRAME_OBSERVER_H_

#include <memory>
#include <optional>

#include "brave/components/youtube_script_injector/renderer/youtube_injector_frame_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace youtube_script_injector {

class COMPONENT_EXPORT(YOUTUBE_SCRIPT_INJECTOR_RENDERER)
    YouTubeRenderFrameObserver : public content::RenderFrameObserver {
 public:
  YouTubeRenderFrameObserver(content::RenderFrame* render_frame,
                             int32_t world_id);
  ~YouTubeRenderFrameObserver() override;
  YouTubeRenderFrameObserver(const YouTubeRenderFrameObserver&) = delete;
  YouTubeRenderFrameObserver& operator=(const YouTubeRenderFrameObserver&) =
      delete;

  // RenderFrameObserver implementation.
  void DidStartNavigation(
      const GURL& url,
      std::optional<blink::WebNavigationType> navigation_type) override;
  void DidFinishLoad() override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  std::unique_ptr<YouTubeInjectorFrameJSHandler> native_javascript_handle_;
  int32_t world_id_;
  GURL url_;
};

}  // namespace youtube_script_injector
#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_RENDERER_YOUTUBE_RENDER_FRAME_OBSERVER_H_
