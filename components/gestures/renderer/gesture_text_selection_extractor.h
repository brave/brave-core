// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_RENDERER_GESTURE_TEXT_SELECTION_EXTRACTOR_H_
#define BRAVE_COMPONENTS_GESTURES_RENDERER_GESTURE_TEXT_SELECTION_EXTRACTOR_H_

#include "brave/components/gestures/core/common/mojom/gesture_text_selection_extractor.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace gestures {

// Scoped to a single `RenderFrame`, self-deleting when it's destroyed (see
// `OnDestruct`).
class GestureTextSelectionExtractor final
    : public mojom::GestureTextSelectionExtractor,
      public content::RenderFrameObserver {
 public:
  GestureTextSelectionExtractor(content::RenderFrame* render_frame,
                                service_manager::BinderRegistry* registry);

  GestureTextSelectionExtractor(const GestureTextSelectionExtractor&) = delete;
  GestureTextSelectionExtractor& operator=(
      const GestureTextSelectionExtractor&) = delete;

  ~GestureTextSelectionExtractor() override;

  // mojom::GestureTextSelectionExtractor:
  void GetSelectedText(
      mojom::GestureTextSelectionExtractor::GetSelectedTextCallback callback)
      override;

 private:
  // content::RenderFrameObserver:
  void OnDestruct() override;

  void BindReceiver(
      mojo::PendingReceiver<mojom::GestureTextSelectionExtractor> receiver);

  mojo::ReceiverSet<mojom::GestureTextSelectionExtractor> receivers_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_RENDERER_GESTURE_TEXT_SELECTION_EXTRACTOR_H_
