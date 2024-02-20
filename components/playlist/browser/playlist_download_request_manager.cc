/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_download_request_manager.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/notreached.h"
#include "base/time/time.h"
#include "content/public/browser/render_frame_host.h"

namespace playlist {

PlaylistDownloadRequestManager::PlaylistDownloadRequestManager(
    MediaDetectorComponentManager* manager)
    : media_detector_component_manager_(manager) {}

PlaylistDownloadRequestManager::~PlaylistDownloadRequestManager() = default;

std::vector<mojom::PlaylistItemPtr>
PlaylistDownloadRequestManager::GetPlaylistItems(base::Value value,
                                                 GURL page_url) {
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
  if (value.is_dict() && value.GetDict().empty()) {
    DVLOG(2) << "No media was detected";
    return items;
  }

  CHECK(value.is_list()) << " Got invalid value after running media detector "
                            "script: Should be list";
  for (const auto& media : value.GetList()) {
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
    if (!CanCacheMedia(item)) {
      LOG(ERROR)
          << __func__
          << "media scheme is not https:// nor blob: that we can cache from";
      continue;
    }

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

bool PlaylistDownloadRequestManager::CanCacheMedia(
    const mojom::PlaylistItemPtr& item) const {
  GURL media_url(item->media_source);
  if (media_url.SchemeIs(url::kHttpsScheme)) {
    return true;
  }

  if (media_url.SchemeIsBlob()) {
    if (item->is_blob_from_media_source) {
      // At this moment, we have a few sites that we can get media files with
      // hacks.
      return media_detector_component_manager_->ShouldHideMediaSrcAPI(
                 media_url) ||
             media_detector_component_manager_->ShouldUseFakeUA(media_url);
    }

    // blob: which is not Media Source
    // TODO(sko) Test and allow this case referring to
    // https://github.com/brave/brave-core/pull/17246
    return false;
  }

  return false;
}

bool PlaylistDownloadRequestManager::
    ShouldExtractMediaFromBackgroundWebContents(
        const mojom::PlaylistItemPtr& item) const {
  GURL media_url(item->media_source);
  if (media_url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }

  if (media_url.SchemeIsBlob() && item->is_blob_from_media_source) {
    CHECK(media_detector_component_manager_->ShouldHideMediaSrcAPI(media_url) ||
          media_detector_component_manager_->ShouldUseFakeUA(media_url));
    return true;
  }

  NOTREACHED_NORETURN()
      << "CanCacheMedia() should be true when this method is called";
}

}  // namespace playlist
