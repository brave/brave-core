/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/html/media/html_video_element.cc"

namespace blink {

void HTMLVideoElement::RequestFullscreen() {
  // Activate frame first so that fullscreen request is allowed.
  // https://source.chromium.org/chromium/chromium/src/+/d6b28888aa75d2d8579808102a50a8aceb9afeaf:third_party/blink/renderer/core/fullscreen/fullscreen.cc;l=351
  LocalFrame::NotifyUserActivation(
      GetDocument().GetFrame(),
      mojom::blink::UserActivationNotificationType::kInteraction,
      /*need_browser_verification*/ false);
  EnterFullscreen();
}

}  // namespace blink
