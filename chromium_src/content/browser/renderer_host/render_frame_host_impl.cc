/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/content/browser/renderer_host/render_frame_host_impl.cc"

namespace content {

void RenderFrameHostImpl::GetImageAt(
    int x,
    int y,
    base::OnceCallback<void(const SkBitmap&)> callback) {
  gfx::PointF point_in_view =
      GetView()->TransformRootPointToViewCoordSpace(gfx::PointF(x, y));
  GetAssociatedLocalFrame()->GetImageAt(
      gfx::Point(point_in_view.x(), point_in_view.y()), std::move(callback));
}

}  // namespace content
