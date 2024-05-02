/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/callback.h"
#include "base/functional/overloaded.h"
#include "base/values.h"
#include "brave/components/playlist/common/playlist_render_frame_observer_helper.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "media/base/media_log_record.h"
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

void PlaylistMediaHandler::Log(
    const std::vector<::media::MediaLogRecord>& events) {
  auto* render_frame_host = media_responder_receivers_.GetCurrentTargetFrame();

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents) {
    return;
  }

  auto url = web_contents->GetLastCommittedURL();
  if (!url.is_valid()) {
    return;
  }

  // auto to_string = [](media::MediaLogRecord::Type type) {
  //   switch (type) {
  //     case media::MediaLogRecord::Type::kMessage:
  //       return "kMessage";
  //     case media::MediaLogRecord::Type::kMediaPropertyChange:
  //       return "kMediaPropertyChange";
  //     case media::MediaLogRecord::Type::kMediaEventTriggered:
  //       return "kMediaEventTriggered";
  //     case media::MediaLogRecord::Type::kMediaStatus:
  //       return "kMediaStatus";
  //   }
  // };

  for (auto& record : events) {
    // DVLOG(-1) << "Player ID: " << record.id
    //           << ", message: " << to_string(record.type) << " (" << url
    //           << "):\n"
    //           << record.params;

    if (auto* string = record.params.FindString("kFrameUrl")) {
      detected_[record.id].Set("pageSrc", *string);
    } else if ((string = record.params.FindString("kFrameTitle"))) {
      detected_[record.id].Set("name", *string);
    } else if ((string = record.params.FindString("url")) &&
               !string->ends_with(".mp3")) {
      detected_[record.id].Set("src", *string);
    }

    if (detected_[record.id].size() == 3) {
      auto* string = detected_[record.id].FindString("src");
      CHECK(string);
      detected_[record.id].Set("srcIsMediaSourceObjectURL",
                               string->starts_with("blob:"));
      auto node = detected_.extract(record.id);
      auto items = ExtractPlaylistItems(
          url, base::Value::List().Append(std::move(node.mapped())));
      if (!items.empty()) {
        OnMediaDetected(std::move(items));
        break;
      }
    }
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistMediaHandler);

}  // namespace playlist
