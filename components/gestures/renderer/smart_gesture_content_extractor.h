// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_RENDERER_SMART_GESTURE_CONTENT_EXTRACTOR_H_
#define BRAVE_COMPONENTS_GESTURES_RENDERER_SMART_GESTURE_CONTENT_EXTRACTOR_H_

#include "brave/components/gestures/core/common/mojom/smart_gesture_content_extractor.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "ui/gfx/geometry/point_f.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace gestures {

// Scoped to a single `RenderFrame`, self-deleting when it's destroyed (see
// `OnDestruct`), same as `GestureTextSelectionExtractor`.
class SmartGestureContentExtractor final
    : public mojom::SmartGestureContentExtractor,
      public content::RenderFrameObserver {
 public:
  SmartGestureContentExtractor(content::RenderFrame* render_frame,
                               service_manager::BinderRegistry* registry);

  SmartGestureContentExtractor(const SmartGestureContentExtractor&) = delete;
  SmartGestureContentExtractor& operator=(const SmartGestureContentExtractor&) =
      delete;

  ~SmartGestureContentExtractor() override;

  // mojom::SmartGestureContentExtractor:
  void GetContentAtPoint(
      const gfx::PointF& point,
      mojom::SmartGestureContentExtractor::GetContentAtPointCallback callback)
      override;

 private:
  // content::RenderFrameObserver:
  void OnDestruct() override;

  void BindReceiver(
      mojo::PendingReceiver<mojom::SmartGestureContentExtractor> receiver);

  mojo::ReceiverSet<mojom::SmartGestureContentExtractor> receivers_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_RENDERER_SMART_GESTURE_CONTENT_EXTRACTOR_H_
