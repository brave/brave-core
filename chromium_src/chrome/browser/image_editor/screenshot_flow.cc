/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/views_features.h"

namespace {

#if !BUILDFLAG(IS_MAC)
// With `views::features::kNativeViewHostManagesLayers` enabled, the
// selection overlay is added as a child of `native_window->layer()`, whose
// bounds are already relative to that same layer. Reusing
// `native_window->bounds()` (which is itself relative to *its* parent, and
// so includes the window's origin) applies that origin a second time,
// offsetting the overlay from the cursor whenever the window isn't at
// (0, 0). See https://github.com/brave/brave-browser/issues/58258.
gfx::Rect AdjustScreenshotOverlayBounds(gfx::Rect bounds) {
  if (base::FeatureList::IsEnabled(
          views::features::kNativeViewHostManagesLayers)) {
    return gfx::Rect(bounds.size());
  }
  return bounds;
}
#endif  // BUILDFLAG(IS_MAC)

}  // namespace

#include <chrome/browser/image_editor/screenshot_flow.cc>
