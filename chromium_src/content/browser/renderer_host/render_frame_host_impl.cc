/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "brave/components/permissions/contexts/brave_puppeteer_permission_context.h"

#include "base/check.h"
#include "base/logging.h"

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

#define BRAVE_RENDER_FRAME_HOST_IMPL_CREATE_NEW_WINDOW \
  params->no_javascript_access = no_javascript_access;

#define BindTrustTokenQueryAnswerer BindTrustTokenQueryAnswerer_ChromiumImpl

#include <content/browser/renderer_host/render_frame_host_impl.cc>

#undef BindTrustTokenQueryAnswerer
#undef BRAVE_RENDER_FRAME_HOST_IMPL_CREATE_NEW_WINDOW
#undef BRAVE_RENDER_FRAME_HOST_IMPL_COMPUTE_NONCE
#undef BRAVE_RENDER_FRAME_HOST_IMPL_IS_THIRD_PARTY_STORAGE_PARTITIONING_ENABLED_CHECK_IF_CAN_BE_DISABLED
#undef BRAVE_RENDER_FRAME_HOST_IMPL_COMPUTE_ISOLATION_INFO_INTERNAL

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
  /*if (!is_main_frame()) {
    // For iframes, check if they have puppeteer permissions and need isolated storage
    auto origin = GetLastCommittedOrigin();

    // Check if this iframe origin is allowed for puppeteer mode
    bool has_puppeteer_permission = false;
    if (GetBrowserContext()) {
      LOG(INFO) << "[PUPPETEER_DEBUG] Checking puppeteer permission for iframe: " << origin;

      // For development, allow origins containing "pompel.me"
      // In production, this would be controlled by the allowlist
      //if (origin.host().find("pompel.me") != std::string::npos) {
      //  has_puppeteer_permission = true;
      //}
      //has_puppeteer_permission = BravePuppeteerPermissionContext::IsOriginAllowedForPuppeteerMode(
      //    GetBrowserContext(), origin);
    }

    if (has_puppeteer_permission) {
      LOG(INFO) << "[PUPPETEER_DEBUG] Setting unique ephemeral storage token for puppeteer iframe: " << origin;
      // Generate unique token for this specific iframe
      ephemeral_storage_token_ = base::UnguessableToken::Create();
      ephemeral_storage_token_set_ = true;
      DVLOG(2) << __func__ << " [PUPPETEER IFRAME] " << origin << " " << ephemeral_storage_token_->ToString();
      return;
    }
    return;
  }*/

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
  // For puppeteer iframes, return their own unique token if set
  if (!is_main_frame() && ephemeral_storage_token_set_) {
    return ephemeral_storage_token_;
  }

  // Otherwise, use the main frame's token
  const RenderFrameHostImpl* main_rfh = this;
  while (main_rfh->parent_) {
    main_rfh = main_rfh->parent_;
  }

  CHECK(main_rfh->ephemeral_storage_token_set_)
      << "RenderFrameHostImpl::SetEphemeralStorageToken wasn't called for the "
         "main frame";
  return main_rfh->ephemeral_storage_token_;
}

void RenderFrameHostImpl::BindTrustTokenQueryAnswerer(
    mojo::PendingReceiver<network::mojom::TrustTokenQueryAnswerer> receiver) {
  mojo::ReportBadMessage(
      "Attempted to get a TrustTokenQueryAnswerer with Private State Tokens "
      "disabled.");
  return;
}

}  // namespace content
