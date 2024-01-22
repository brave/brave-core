/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_handler.h"

#include "brave/components/playlist/browser/playlist_service.h"

namespace playlist {

PlaylistMediaHandler::PlaylistMediaHandler(
    content::GlobalRenderFrameHostId frame_id,
    const base::WeakPtr<PlaylistService>& service)
    : frame_id_(frame_id), service_(service) {
  DVLOG(2) << __FUNCTION__ << " " << frame_id_;
}

PlaylistMediaHandler::~PlaylistMediaHandler() {
  DVLOG(2) << __FUNCTION__ << " " << frame_id_;
}

void PlaylistMediaHandler::OnMediaUpdatedFromRenderFrame() {
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
