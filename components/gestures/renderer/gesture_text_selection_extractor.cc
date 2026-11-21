// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/renderer/gesture_text_selection_extractor.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/gestures/renderer/gesture_selected_text_util.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace gestures {

GestureTextSelectionExtractor::GestureTextSelectionExtractor(
    content::RenderFrame* render_frame,
    service_manager::BinderRegistry* registry)
    : content::RenderFrameObserver(render_frame) {
  // Being a `RenderFrameObserver`, this object is scoped to `render_frame`;
  // `registry` is too, so `base::Unretained` is safe here.
  registry->AddInterface(base::BindRepeating(
      &GestureTextSelectionExtractor::BindReceiver, base::Unretained(this)));
}

GestureTextSelectionExtractor::~GestureTextSelectionExtractor() = default;

void GestureTextSelectionExtractor::GetSelectedText(
    mojom::GestureTextSelectionExtractor::GetSelectedTextCallback callback) {
  std::move(callback).Run(gestures::GetSelectedText(render_frame()));
}

void GestureTextSelectionExtractor::OnDestruct() {
  delete this;
}

void GestureTextSelectionExtractor::BindReceiver(
    mojo::PendingReceiver<mojom::GestureTextSelectionExtractor> receiver) {
  receivers_.Add(this, std::move(receiver));
}

}  // namespace gestures
