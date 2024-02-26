/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_handler.h"

#include <string>

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
  // TODO(sszaloki): do we have to do a service check here?
  // auto* playlist_service =
  //     PlaylistServiceFactory::GetForBrowserContext(
  //         rfh->GetBrowserContext());
  // if (!playlist_service) {
  //   // We don't support playlist on OTR profile.
  //   return;
  // }

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
  std::visit(
      [](auto& on_media_detected_callback) {
        CHECK(on_media_detected_callback);
      },
      on_media_detected_callback_);
}

void PlaylistMediaHandler::OnMediaDetected(base::Value::List media) {
  CHECK(!media.empty())
      << "This invariant should be maintained by the renderer!";

  // TODO(sszaloki):
  // Should we use RenderFrameHost::GetGlobalFrameToken() instead?
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

  static const std::string kFunction = __FUNCTION__;
  std::visit(
      base::Overloaded{
          [&](base::OnceCallback<Signature>& on_media_detected_callback) {
            if (on_media_detected_callback) {
              DVLOG(2) << render_frame_host_id << " - " << kFunction
                       << " (background):\n"
                       << media;

              std::move(on_media_detected_callback)
                  .Run(std::move(media), web_contents->GetLastCommittedURL());
            }
          },
          [&](base::RepeatingCallback<Signature>& on_media_detected_callback) {
            DVLOG(2) << render_frame_host_id << " - " << kFunction << ":\n"
                     << media;

            on_media_detected_callback.Run(std::move(media),
                                           web_contents->GetLastCommittedURL());
          }},
      on_media_detected_callback_);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistMediaHandler);

}  // namespace playlist
