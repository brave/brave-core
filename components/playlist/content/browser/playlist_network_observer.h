/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_NETWORK_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_NETWORK_OBSERVER_H_

#include <string>
#include <string_view>

#include "base/containers/hashing_lru_cache.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/supports_user_data.h"
#include "brave/components/playlist/content/browser/media_stream_classifier.h"
#include "content/public/browser/global_routing_id.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace playlist {

struct MediaResponseInfo {
  GURL url;
  MediaKind kind;
  std::string mime_type;
};

// Receives media responses spotted in the network stack and hands them to
// per-tab consumers. Lives on the BrowserContext because the network proxy
// that feeds it has no WebContents; consumers key off the frame token.
//
// UI thread only.
class PlaylistNetworkObserver : public base::SupportsUserData::Data {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnMediaResponseObserved(
        const content::GlobalRenderFrameHostToken& frame_token,
        const MediaResponseInfo& info) = 0;
  };

  PlaylistNetworkObserver();
  PlaylistNetworkObserver(const PlaylistNetworkObserver&) = delete;
  PlaylistNetworkObserver& operator=(const PlaylistNetworkObserver&) = delete;
  ~PlaylistNetworkObserver() override;

  static PlaylistNetworkObserver* GetOrCreate(
      content::BrowserContext* browser_context);
  static PlaylistNetworkObserver* Get(content::BrowserContext* browser_context);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void OnMediaResponse(const content::GlobalRenderFrameHostToken& frame_token,
                       const MediaResponseInfo& info);

 private:
  bool ShouldNotify(const content::GlobalRenderFrameHostToken& frame_token,
                    const MediaResponseInfo& info);

  // Suppresses repeat notifications. Streaming sites emit hundreds of segment
  // responses per minute, so segments are keyed by origin rather than by URL -
  // one notification per stream, not per fragment.
  base::HashingLRUCacheSet<std::string> recently_notified_;

  base::ObserverList<Observer> observers_;
};

// Entry point for the network stack. Cheap and safe to call for every response;
// does nothing unless Playlist V2 is enabled and the response looks like media.
void MaybeNotifyMediaResponse(
    content::BrowserContext* browser_context,
    const content::GlobalRenderFrameHostToken& frame_token,
    const GURL& url,
    std::string_view mime_type,
    network::mojom::RequestDestination destination);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_NETWORK_OBSERVER_H_
