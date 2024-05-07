/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/content/browser/media/media_web_contents_observer.cc"

#include "base/ranges/algorithm.h"

namespace content {

void MediaWebContentsObserver::MediaPlayerObserverHostImpl::OnMediaLoaded(
    const GURL& url,
    bool is_media_source,
    double duration) {
  url_ = url;
  is_media_source_ = is_media_source;
  duration_ = duration;
}

const GURL& MediaWebContentsObserver::MediaPlayerObserverHostImpl::GetUrl()
    const {
  return url_;
}

bool MediaWebContentsObserver::MediaPlayerObserverHostImpl::GetIsMediaSource()
    const {
  return is_media_source_;
}

double MediaWebContentsObserver::MediaPlayerObserverHostImpl::GetDuration()
    const {
  return duration_;
}

base::flat_map<MediaPlayerId, std::tuple<GURL, bool, double>>
MediaWebContentsObserver::GetMediaMetadataByMediaPlayerIds() const {
  decltype(GetMediaMetadataByMediaPlayerIds()) metadata;

  base::ranges::transform(
      media_player_observer_hosts_, std::inserter(metadata, metadata.end()),
      [](const auto& pair) {
        const auto& [media_player_id, observer_host] = pair;
        return std::pair(media_player_id,
                         std::tuple(observer_host->GetUrl(),
                                    observer_host->GetIsMediaSource(),
                                    observer_host->GetDuration()));
      });

  return metadata;
}

}  // namespace content
