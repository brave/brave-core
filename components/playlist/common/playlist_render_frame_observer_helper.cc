/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/common/playlist_render_frame_observer_helper.h"

#include <optional>
#include <string>
#include <utility>

#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "base/token.h"
#include "url/gurl.h"

namespace playlist {

std::vector<mojom::PlaylistItemPtr> ExtractPlaylistItems(
    const GURL& url,
    base::Value::List list) {
  /* Expected input:
    [
      {
        "mimeType": "video" | "audio",
        "name": string,
        "pageSrc": url,
        "pageTitle": string,
        "src": url,
        "srcIsMediaSourceObjectURL": boolean,
        "thumbnail": url | undefined,
        "duration": double | undefined,
        "author": string | undefined
      }
    ]
  */
  std::vector<mojom::PlaylistItemPtr> items;

  for (const auto& media : list) {
    if (!media.is_dict()) {
      LOG(ERROR) << __FUNCTION__ << ": media is not a dict";
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
      LOG(ERROR) << __FUNCTION__ << ": media is missing required fields";
      continue;
    }

    // nullable data
    auto* thumbnail = media_dict.FindString("thumbnail");
    auto* author = media_dict.FindString("author");
    auto duration = media_dict.FindDouble("duration");

    auto item = mojom::PlaylistItem::New();
    item->id = base::Token::CreateRandom().ToString();
    item->page_source = url;
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
        LOG(ERROR) << __FUNCTION__ << "thumbnail scheme is not https://";
        thumbnail = nullptr;
      }
    }

    if (duration) {
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

  DVLOG(2) << __FUNCTION__ << ": successfully converted " << items.size()
           << " items";

  return items;
}

}  // namespace playlist
