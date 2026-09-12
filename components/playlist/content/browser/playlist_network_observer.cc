/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_network_observer.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "brave/components/playlist/core/common/features.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "url/origin.h"

namespace playlist {

namespace {

// Enough to cover the distinct media origins of a busy tab without letting a
// hostile page grow this without bound.
constexpr size_t kRecentlyNotifiedCapacity = 256;

constexpr char kUserDataKey[] = "playlist::PlaylistNetworkObserver";

}  // namespace

PlaylistNetworkObserver::PlaylistNetworkObserver()
    : recently_notified_(kRecentlyNotifiedCapacity) {}

PlaylistNetworkObserver::~PlaylistNetworkObserver() = default;

// static
PlaylistNetworkObserver* PlaylistNetworkObserver::GetOrCreate(
    content::BrowserContext* browser_context) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(browser_context);

  if (auto* observer = Get(browser_context)) {
    return observer;
  }

  auto observer = std::make_unique<PlaylistNetworkObserver>();
  auto* observer_ptr = observer.get();
  browser_context->SetUserData(kUserDataKey, std::move(observer));
  return observer_ptr;
}

// static
PlaylistNetworkObserver* PlaylistNetworkObserver::Get(
    content::BrowserContext* browser_context) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(browser_context);
  return static_cast<PlaylistNetworkObserver*>(
      browser_context->GetUserData(kUserDataKey));
}

void PlaylistNetworkObserver::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void PlaylistNetworkObserver::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void PlaylistNetworkObserver::OnMediaResponse(
    const content::GlobalRenderFrameHostToken& frame_token,
    const MediaResponseInfo& info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!ShouldNotify(frame_token, info)) {
    return;
  }

  for (auto& observer : observers_) {
    observer.OnMediaResponseObserved(frame_token, info);
  }
}

bool PlaylistNetworkObserver::ShouldNotify(
    const content::GlobalRenderFrameHostToken& frame_token,
    const MediaResponseInfo& info) {
  // Segments are collapsed per origin: knowing that a stream exists is the
  // whole signal, and repeating it for every fragment would flood consumers.
  const std::string what = info.kind == MediaKind::kSegment
                               ? url::Origin::Create(info.url).Serialize()
                               : info.url.spec();
  std::string key =
      base::StrCat({frame_token.frame_token.ToString(), "|", what});

  if (recently_notified_.Get(key) != recently_notified_.end()) {
    return false;
  }

  recently_notified_.Put(std::move(key));
  return true;
}

void MaybeNotifyMediaResponse(
    content::BrowserContext* browser_context,
    const content::GlobalRenderFrameHostToken& frame_token,
    const GURL& url,
    std::string_view mime_type,
    network::mojom::RequestDestination destination) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!base::FeatureList::IsEnabled(features::kPlaylistServiceV2) ||
      !base::FeatureList::IsEnabled(features::kPlaylist)) {
    return;
  }

  // Playlist is unavailable in private windows, so don't watch their traffic.
  if (!browser_context || browser_context->IsOffTheRecord()) {
    return;
  }

  const auto kind = ClassifyMediaResponse(url, mime_type, destination);
  if (!kind) {
    return;
  }

  PlaylistNetworkObserver::GetOrCreate(browser_context)
      ->OnMediaResponse(
          frame_token,
          MediaResponseInfo{
              .url = url, .kind = *kind, .mime_type = std::string(mime_type)});
}

}  // namespace playlist
