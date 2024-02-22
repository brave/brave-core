/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_handler.h"

#include "brave/components/playlist/browser/playlist_background_webcontents_helper.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "content/public/browser/web_contents.h"

namespace playlist {

PlaylistMediaHandler::PlaylistMediaHandler(PlaylistService* service,
                                           content::WebContents* contents)
    : service_(service), media_responder_receivers_(contents, this) {}

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

  service_->OnMediaDetected(std::move(media),
                            web_contents->GetLastCommittedURL());

  if (background_webcontents_helper) {
    // TODO(sszaloki): bloah...
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(
            std::move(*background_webcontents_helper).GetSuccessCallback(),
            true));
  }
}

void PlaylistMediaHandler::BindMediaResponder(
    mojo::PendingAssociatedReceiver<mojom::PlaylistMediaResponder> receiver,
    content::RenderFrameHost* render_frame_host) {
  media_responder_receivers_.Bind(render_frame_host, std::move(receiver));
}

}  // namespace playlist
