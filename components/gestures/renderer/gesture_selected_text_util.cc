// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/renderer/gesture_selected_text_util.h"

#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace gestures {

std::string GetSelectedText(content::RenderFrame* render_frame) {
  blink::WebLocalFrame* const web_frame =
      render_frame ? render_frame->GetWebFrame() : nullptr;
  return web_frame ? web_frame->SelectionAsText().Utf8() : "";
}

}  // namespace gestures
