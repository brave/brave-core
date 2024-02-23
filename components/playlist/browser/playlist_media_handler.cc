/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_handler.h"

#include "brave/components/playlist/browser/playlist_background_webcontents_helper.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "content/public/browser/web_contents.h"

namespace playlist {

PlaylistMediaHandler::PlaylistMediaHandler(
    content::WebContents* contents,
    base::RepeatingCallback<void(base::Value, const GURL&)> on_media_detected)
    : media_responder_receivers_(contents, this),
      on_media_detected_(std::move(on_media_detected)) {}

PlaylistMediaHandler::~PlaylistMediaHandler() = default;

void PlaylistMediaHandler::OnMediaDetected(base::Value media) {
  const auto render_frame_host_id =
      media_responder_receivers_.GetCurrentTargetFrame()->GetGlobalId();

  auto* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id);
  if (!render_frame_host) {
    return;
  }

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }

  auto* background_webcontents_helper =
      PlaylistBackgroundWebContentsHelper::FromWebContents(web_contents);

  DVLOG(2) << render_frame_host_id << " - " << __FUNCTION__
           << (background_webcontents_helper ? " (background)" : "") << ":\n"
           << media;

  on_media_detected_.Run(std::move(media), web_contents->GetLastCommittedURL());

  if (background_webcontents_helper) {
    // TODO(sszaloki): bloah...
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(
            std::move(*background_webcontents_helper).GetSuccessCallback(),
            true));
  }
}

void PlaylistMediaHandler::BindMediaResponderReceiver(
    mojo::PendingAssociatedReceiver<mojom::PlaylistMediaResponder> receiver,
    content::RenderFrameHost* render_frame_host) {
  media_responder_receivers_.Bind(render_frame_host, std::move(receiver));
}

}  // namespace playlist
