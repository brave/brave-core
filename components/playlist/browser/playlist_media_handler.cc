/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/callback.h"
#include "base/functional/overloaded.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace playlist {

PlaylistMediaHandler::~PlaylistMediaHandler() = default;

void PlaylistMediaHandler::BindMediaResponderReceiver(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingAssociatedReceiver<mojom::PlaylistMediaResponder> receiver) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }

  if (auto* media_handler =
          PlaylistMediaHandler::FromWebContents(web_contents)) {
    media_handler->media_responder_receivers_.Bind(render_frame_host,
                                                   std::move(receiver));
  }
}

PlaylistMediaHandler::PlaylistMediaHandler(
    content::WebContents* web_contents,
    OnMediaDetectedCallback on_media_detected_callback)
    : content::WebContentsUserData<PlaylistMediaHandler>(*web_contents),
      media_responder_receivers_(web_contents, this),
      on_media_detected_callback_(std::move(on_media_detected_callback)) {
  CHECK(std::visit(
      [](auto& on_media_detected_callback) {
        return !on_media_detected_callback.is_null();
      },
      on_media_detected_callback_));
}

void PlaylistMediaHandler::OnMediaDetected(
    std::vector<mojom::PlaylistItemPtr> items) {
  CHECK(!items.empty())
      << "This invariant should be maintained by the renderer!";

  auto* render_frame_host = media_responder_receivers_.GetCurrentTargetFrame();

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }

  auto url = web_contents->GetLastCommittedURL();
  std::visit(
      base::Overloaded{[&](OnceCallback& on_media_detected_callback) {
                         if (on_media_detected_callback) {
                           std::move(on_media_detected_callback)
                               .Run(std::move(url), std::move(items));
                         }
                       },
                       [&](RepeatingCallback& on_media_detected_callback) {
                         on_media_detected_callback.Run(std::move(url),
                                                        std::move(items));
                       }},
      on_media_detected_callback_);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistMediaHandler);

}  // namespace playlist
