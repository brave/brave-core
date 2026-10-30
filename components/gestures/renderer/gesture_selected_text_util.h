// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_RENDERER_GESTURE_SELECTED_TEXT_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_RENDERER_GESTURE_SELECTED_TEXT_UTIL_H_

#include <string>

namespace content {
class RenderFrame;
}  // namespace content

namespace gestures {

// `render_frame`'s currently selected text, or an empty string if it's gone
// (e.g. the frame was already detached) or nothing is selected.
std::string GetSelectedText(content::RenderFrame* render_frame);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_RENDERER_GESTURE_SELECTED_TEXT_UTIL_H_
