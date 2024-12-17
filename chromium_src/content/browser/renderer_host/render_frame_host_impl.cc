/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/render_frame_host_impl.h"

#define BRAVE_RENDER_FRAME_HOST_IMPL_COMPUTE_ISOLATION_INFO_INTERNAL \
  SetEphemeralStorageToken(top_frame_origin);

#define BRAVE_RENDER_FRAME_HOST_IMPL_COMPUTE_NONCE                 \
  if (auto ephemeral_storage_token = GetEphemeralStorageToken()) { \
    return *ephemeral_storage_token;                               \
  }

#define BRAVE_RENDER_FRAME_HOST_IMPL_IS_THIRD_PARTY_STORAGE_PARTITIONING_ENABLED_CHECK_IF_CAN_BE_DISABLED \
  if (GetContentClient()                                                                                  \
          ->browser()                                                                                     \
          ->CanThirdPartyStoragePartitioningBeDisabled(                                                   \
              GetBrowserContext(),                                                                        \
              main_frame_for_storage_partitioning->GetLastCommittedOrigin()))

#include "src/content/browser/renderer_host/render_frame_host_impl.cc"

#undef BRAVE_RENDER_FRAME_HOST_IMPL_COMPUTE_ISOLATION_INFO_INTERNAL
#undef BRAVE_RENDER_FRAME_HOST_IMPL_COMPUTE_NONCE
#undef BRAVE_RENDER_FRAME_HOST_IMPL_IS_THIRD_PARTY_STORAGE_PARTITIONING_ENABLED_CHECK_IF_CAN_BE_DISABLED

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

void RenderFrameHostImpl::SetEphemeralStorageToken(
    const url::Origin& top_frame_origin) {
  if (!is_main_frame()) {
    return;
  }

  ephemeral_storage_token_ =
      GetContentClient()->browser()->GetEphemeralStorageToken(this,
                                                              top_frame_origin);
  ephemeral_storage_token_set_ = true;

  DVLOG(2) << __func__ << " " << top_frame_origin << " "
           << (ephemeral_storage_token_ ? ephemeral_storage_token_->ToString()
                                        : std::string());
}

std::optional<base::UnguessableToken>
RenderFrameHostImpl::GetEphemeralStorageToken() const {
  const RenderFrameHostImpl* main_rfh = this;
  while (main_rfh->parent_) {
    main_rfh = main_rfh->parent_;
  }

  CHECK(main_rfh->ephemeral_storage_token_set_)
      << "RenderFrameHostImpl::SetEphemeralStorageToken wasn't called for the "
         "main frame";
  return main_rfh->ephemeral_storage_token_;
}

}  // namespace content
