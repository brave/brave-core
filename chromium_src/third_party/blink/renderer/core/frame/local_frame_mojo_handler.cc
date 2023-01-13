/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/frame/local_frame_mojo_handler.cc"

namespace blink {

void LocalFrameMojoHandler::GetImageAt(const gfx::Point& window_point,
                                       GetImageAtCallback callback) {
  gfx::Point viewport_position =
      frame_->GetWidgetForLocalRoot()->DIPsToRoundedBlinkSpace(window_point);
  std::move(callback).Run(frame_->GetImageAtViewportPoint(viewport_position));
}

}  // namespace blink
