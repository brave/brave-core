// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_RENDERER_SMART_GESTURE_CONTENT_EXTRACTOR_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_RENDERER_SMART_GESTURE_CONTENT_EXTRACTOR_UTIL_H_

#include "brave/components/gestures/core/common/mojom/smart_gesture_content_extractor.mojom.h"

namespace blink {
class WebHitTestResult;
}  // namespace blink

namespace content {
class RenderFrame;
}  // namespace content

namespace gestures {

// Tries, in order, to extract a link URL, image URL, or selected text at
// `hit_test_result`; null if none apply.
mojom::SmartGestureContentPtr ExtractSmartGestureContent(
    const blink::WebHitTestResult& hit_test_result,
    content::RenderFrame* render_frame);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_RENDERER_SMART_GESTURE_CONTENT_EXTRACTOR_UTIL_H_
