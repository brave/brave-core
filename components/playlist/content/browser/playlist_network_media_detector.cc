/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_network_media_detector.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/token.h"
#include "brave/components/playlist/content/browser/playlist_constants.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace playlist {

namespace {

// How long to wait after finding media before handing it over. MediaSession
// metadata arrives once playback starts, which is after the first media
// response, so emitting immediately would produce untitled items.
constexpr base::TimeDelta kEmitDelay = base::Seconds(1);

}  // namespace

PlaylistNetworkMediaDetector::PlaylistNetworkMediaDetector(
    content::WebContents* web_contents,
    MediaDetectedCallback on_media_detected)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<PlaylistNetworkMediaDetector>(*web_contents),
      media_session_observer_(std::make_unique<PlaylistMediaSessionObserver>(
          web_contents,
          base::BindRepeating(&PlaylistNetworkMediaDetector::OnMetadataChanged,
                              base::Unretained(this)))),
      network_observer_(PlaylistNetworkObserver::GetOrCreate(
          web_contents->GetBrowserContext())),
      on_media_detected_(std::move(on_media_detected)) {
  CHECK(on_media_detected_);
  network_observer_->AddObserver(this);
}

PlaylistNetworkMediaDetector::~PlaylistNetworkMediaDetector() {
  network_observer_->RemoveObserver(this);
}

void PlaylistNetworkMediaDetector::OnMediaResponseObserved(
    const content::GlobalRenderFrameHostToken& frame_token,
    const MediaResponseInfo& info) {
  // The network observer is per profile, so most responses belong to other
  // tabs.
  auto* render_frame_host =
      content::RenderFrameHost::FromFrameToken(frame_token);
  if (!render_frame_host || content::WebContents::FromRenderFrameHost(
                                render_frame_host) != web_contents()) {
    return;
  }

  // V2 deliberately leaves YouTube to the legacy detector and its
  // MediaSource-suppressed background resolution path.
  if (IsYoutubeLegacyPlaylistSite(web_contents()->GetLastCommittedURL())) {
    return;
  }

  // Bare MSE segments are recognized but not actionable on their own: they
  // only say that a stream exists, not where its manifest is.
  if (info.kind == MediaKind::kSegment) {
    return;
  }

  if (emitted_media_.contains(info.url) ||
      std::ranges::contains(pending_media_, info.url)) {
    return;
  }

  pending_media_.push_back(info.url);
  emit_timer_.Start(FROM_HERE, kEmitDelay,
                    base::BindOnce(&PlaylistNetworkMediaDetector::Emit,
                                   base::Unretained(this)));
}

void PlaylistNetworkMediaDetector::OnMetadataChanged() {
  if (pending_media_.empty()) {
    return;
  }

  // Metadata just improved, so restart the wait - a title arriving 200ms from
  // now is worth more than emitting 200ms sooner.
  emit_timer_.Start(FROM_HERE, kEmitDelay,
                    base::BindOnce(&PlaylistNetworkMediaDetector::Emit,
                                   base::Unretained(this)));
}

void PlaylistNetworkMediaDetector::Emit() {
  if (pending_media_.empty()) {
    return;
  }

  const GURL page_url = web_contents()->GetLastCommittedURL();
  std::vector<mojom::PlaylistItemPtr> items;
  for (const auto& media_url : pending_media_) {
    items.push_back(MakeItem(page_url, media_url));
    emitted_media_.insert(media_url);
  }
  pending_media_.clear();

  on_media_detected_.Run(page_url, std::move(items));
}

mojom::PlaylistItemPtr PlaylistNetworkMediaDetector::MakeItem(
    const GURL& page_url,
    const GURL& media_url) const {
  const auto& metadata = media_session_observer_->metadata();

  auto item = mojom::PlaylistItem::New();
  item->id = base::Token::CreateRandom().ToString();
  item->page_source = page_url;
  item->page_redirected = page_url;
  item->media_source = media_url;
  item->media_path = media_url;
  item->is_blob_from_media_source = false;

  // Fall back to the tab title, which is what a user would call this anyway.
  item->name = base::UTF16ToUTF8(
      metadata.title.empty() ? web_contents()->GetTitle() : metadata.title);
  item->author = base::UTF16ToUTF8(metadata.artist);

  if (!metadata.duration.is_zero()) {
    item->duration = base::TimeDeltaToValue(metadata.duration).GetString();
  }
  if (!metadata.artwork.is_empty()) {
    item->thumbnail_source = metadata.artwork;
    item->thumbnail_path = metadata.artwork;
  }

  return item;
}

void PlaylistNetworkMediaDetector::PrimaryPageChanged(content::Page& page) {
  emit_timer_.Stop();
  pending_media_.clear();
  emitted_media_.clear();
  media_session_observer_->Reset();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistNetworkMediaDetector);

}  // namespace playlist
