// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_CONTENT_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_CONTENT_INFO_H_

#include <optional>
#include <string>

#include "brave/components/gestures/core/common/smart/smart_gesture_content_type.h"
#include "third_party/blink/public/common/tokens/tokens.h"
#include "ui/gfx/geometry/point_f.h"
#include "url/origin.h"

namespace gestures {

// The link/image/selection found at a smart gesture's start point, captured
// once and carried through to whichever action actually runs.
struct SmartGestureContentInfo final {
  SmartGestureContentType type = SmartGestureContentType::kUndefined;

  // The link/image URL for `kLink`/`kImage`, or the selected text for
  // `kSelectedText`.
  std::string value;

  // Where `type`/`value` were found; only `kSaveImage`/`kCopyImage` need
  // this, since those act on a point rather than a URL.
  gfx::PointF point;

  // The hit-tested frame's identity, carried so a navigation built from a
  // page-supplied `kLink`/`kImage` is attributed to that frame, not the
  // browser or the tab's main frame.
  url::Origin origin;
  std::optional<blink::LocalFrameToken> frame_token;
  int process_id = -1;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_CONTENT_INFO_H_
