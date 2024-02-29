/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_handler.h"

#include <string>

#include "base/check.h"
#include "base/functional/callback.h"
#include "base/functional/overloaded.h"
#include "base/json/values_util.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace playlist {

std::vector<mojom::PlaylistItemPtr> GetPlaylistItems(base::Value::List list,
                                                     const GURL& page_url) {
  /* Expected output:
    [
      {
        "mimeType": "video" | "audio",
        "name": string,
        "pageSrc": url,
        "pageTitle": string
        "src": url
        "srcIsMediaSourceObjectURL": boolean,
        "thumbnail": url | undefined
        "duration": double | undefined
        "author": string | undefined
      }
    ]
  */

  std::vector<mojom::PlaylistItemPtr> items;
  for (const auto& media : list) {
    if (!media.is_dict()) {
      LOG(ERROR) << __func__ << " Got invalid item";
      continue;
    }

    const auto& media_dict = media.GetDict();

    auto* name = media_dict.FindString("name");
    auto* page_title = media_dict.FindString("pageTitle");
    auto* page_source = media_dict.FindString("pageSrc");
    auto* mime_type = media_dict.FindString("mimeType");
    auto* src = media_dict.FindString("src");
    auto is_blob_from_media_source =
        media_dict.FindBool("srcIsMediaSourceObjectURL");
    if (!name || !page_source || !page_title || !mime_type || !src ||
        !is_blob_from_media_source) {
      LOG(ERROR) << __func__ << " required fields are not satisfied";
      continue;
    }

    // nullable data
    auto* thumbnail = media_dict.FindString("thumbnail");
    auto* author = media_dict.FindString("author");
    auto duration = media_dict.FindDouble("duration");

    auto item = mojom::PlaylistItem::New();
    item->id = base::Token::CreateRandom().ToString();
    item->page_source = page_url;
    item->page_redirected = GURL(*page_source);
    item->name = *name;
    // URL data
    GURL media_url(*src);
    if (!media_url.SchemeIs(url::kHttpsScheme) && !media_url.SchemeIsBlob()) {
      continue;
    }

    if (media_url.SchemeIsBlob() &&
        !GURL(media_url.path()).SchemeIs(url::kHttpsScheme)) {
      // Double checking if the blob: is followed by https:// scheme.
      // https://github.com/brave/playlist-component/pull/39#discussion_r1445408827
      continue;
    }

    item->media_source = media_url;
    item->media_path = media_url;
    item->is_blob_from_media_source = *is_blob_from_media_source;

    if (thumbnail) {
      if (GURL thumbnail_url(*thumbnail);
          !thumbnail_url.SchemeIs(url::kHttpsScheme)) {
        LOG(ERROR) << __func__ << "thumbnail scheme is not https://";
        thumbnail = nullptr;
      }
    }

    if (duration.has_value()) {
      item->duration =
          base::TimeDeltaToValue(base::Seconds(*duration)).GetString();
    }
    if (thumbnail) {
      item->thumbnail_source = GURL(*thumbnail);
      item->thumbnail_path = GURL(*thumbnail);
    }
    if (author) {
      item->author = *author;
    }
    items.push_back(std::move(item));
  }

  DVLOG(2) << __func__ << " Media detection result size: " << items.size();

  return items;
}

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
  const auto url = web_contents->GetLastCommittedURL();
  auto items = GetPlaylistItems(std::move(media), url);
  std::visit(
      base::Overloaded{
          [&](base::OnceCallback<Signature>& on_media_detected_callback) {
            if (on_media_detected_callback) {
              DVLOG(2) << render_frame_host_id << " - " << kFunction
                       << " (background):\n"
                       << media;

              std::move(on_media_detected_callback).Run(std::move(items), url);
            }
          },
          [&](base::RepeatingCallback<Signature>& on_media_detected_callback) {
            DVLOG(2) << render_frame_host_id << " - " << kFunction << ":\n"
                     << media;

            on_media_detected_callback.Run(std::move(items), url);
          }},
      on_media_detected_callback_);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistMediaHandler);

}  // namespace playlist
