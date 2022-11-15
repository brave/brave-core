// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"

namespace speedreader {

class SpeedreaderRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit SpeedreaderRenderFrameObserver(content::RenderFrame* render_frame);
  SpeedreaderRenderFrameObserver(const SpeedreaderRenderFrameObserver&) =
      delete;
  SpeedreaderRenderFrameObserver& operator=(
      const SpeedreaderRenderFrameObserver&) = delete;
  ~SpeedreaderRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  void DidStartNavigation(
      const GURL& url,
      absl::optional<blink::WebNavigationType> navigation_type) override;
  void DidClearWindowObject() override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  bool is_speedreadable_url_ = false;
  base::WeakPtrFactory<SpeedreaderRenderFrameObserver> weak_ptr_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_RENDER_FRAME_OBSERVER_H_
