/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_render_frame_browser_client.h"

#include "brave/components/playlist/browser/playlist_service.h"

namespace playlist {

PlaylistRenderFrameBrowserClient::PlaylistRenderFrameBrowserClient(
    content::GlobalRenderFrameHostId frame_id,
    const base::WeakPtr<PlaylistService>& service)
    : frame_id_(frame_id), service_(service) {
  DVLOG(2) << __FUNCTION__ << " " << frame_id_;
}

PlaylistRenderFrameBrowserClient::~PlaylistRenderFrameBrowserClient() {
  DVLOG(2) << __FUNCTION__ << " " << frame_id_;
}

void PlaylistRenderFrameBrowserClient::OnMediaUpdatedFromRenderFrame() {
  DVLOG(2) << __FUNCTION__ << " " << frame_id_;
  auto* render_frame_host = content::RenderFrameHost::FromID(frame_id_);
  if (!render_frame_host) {
    return;
  }

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }

  if (service_) {
    service_->OnMediaUpdatedFromContents(web_contents);
  }
}

}  // namespace playlist
