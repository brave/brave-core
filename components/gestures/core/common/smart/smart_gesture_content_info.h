// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_CONTENT_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_CONTENT_INFO_H_

#include <string>

#include "brave/components/gestures/core/common/smart/smart_gesture_content_type.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// The link/image/selection found at a smart gesture's start point (see
// `SmartGestureContentExtractor`), captured once at `GestureDrawSession::
// Start` and carried through to whichever action actually runs.
struct SmartGestureContentInfo {
  SmartGestureContentType type = SmartGestureContentType::kUndefined;

  // The link/image URL for `kLink`/`kImage`, or the selected text for
  // `kSelectedText`.
  std::string value;

  // Where `type`/`value` were found, in the page's root coordinate space.
  // Only `kSaveImage`/`kCopyImage` need this, for `RenderFrameHost::
  // SaveImageAt`/`CopyImageAt`, which (like the native context menu) act on
  // a point rather than a URL.
  gfx::PointF point;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_CONTENT_INFO_H_
