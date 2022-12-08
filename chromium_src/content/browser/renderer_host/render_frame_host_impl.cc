/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/render_frame_host_impl.h"

#include "brave/net/query_filter/query_filter.h"

// This is for renderer-initiated navigations (e.g. clicking on a link).
#define BRAVE_RENDER_FRAME_HOST_IMPL_BEGIN_NAVIGATION    \
  net::query_filter::MaybeRemoveTrackingQueryParameters( \
      validated_common_params->initiator_origin,         \
      validated_common_params->url);

#include "src/content/browser/renderer_host/render_frame_host_impl.cc"

#undef BRAVE_RENDER_FRAME_HOST_IMPL_BEGIN_NAVIGATION

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
