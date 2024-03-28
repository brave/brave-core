/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTSES_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTSES_H_

#include <map>
#include <memory>
#include <vector>

#include "base/containers/unique_ptr_adapters.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/playlist/browser/playlist_media_handler.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "url/gurl.h"

FORWARD_DECLARE_TEST(CosmeticFilteringPlaylistFlagEnabledTest,
                     AllowCosmeticFiltering);

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace playlist {

class PlaylistService;

// `PlaylistBackgroundWebContentses` fulfills background `WebContents` requests.
// After creating the background `WebContents`, it waits 10 seconds for the
// first non-empty media list to arrive. On receiving the media, or if the timer
// goes off (whichever happens first), it destructs the background
// `WebContents`, and calls the provided callback with the result.
// It overrides the user agent if `features::kPlaylistFakeUA` is enabled,
// or uses a static look-up table to decide if it has to otherwise.
class PlaylistBackgroundWebContentses final {
 public:
  PlaylistBackgroundWebContentses(content::BrowserContext* context,
                                  PlaylistService* service);
  PlaylistBackgroundWebContentses(const PlaylistBackgroundWebContentses&) =
      delete;
  PlaylistBackgroundWebContentses& operator=(
      const PlaylistBackgroundWebContentses&) = delete;
  ~PlaylistBackgroundWebContentses();

  void Add(const GURL& url,
           PlaylistMediaHandler::OnceCallback on_media_detected_callback,
           base::TimeDelta timeout = base::Seconds(10));

  void Reset();

 private:
  FRIEND_TEST_ALL_PREFIXES(::CosmeticFilteringPlaylistFlagEnabledTest,
                           AllowCosmeticFiltering);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBackgroundWebContentsTest,
                           ExtractPlaylistItemsInTheBackground);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBackgroundWebContentsTest,
                           UserAgentOverride);

  void Remove(content::WebContents* web_contents,
              PlaylistMediaHandler::OnceCallback on_media_detected_callback,
              GURL url,
              std::vector<mojom::PlaylistItemPtr> items);

  // used by
  // PlaylistBackgroundWebContentsTest.ExtractPlaylistItemsInTheBackground, and
  // PlaylistBackgroundWebContentsTest.UserAgentOverride
  content::WebContents& web_contents() const {
    CHECK(background_web_contentses_.size() == 1);
    return *background_web_contentses_.cbegin()->first;
  }

  raw_ptr<content::BrowserContext> context_;
  raw_ptr<PlaylistService> service_;
  std::map<std::unique_ptr<content::WebContents>,
           base::OneShotTimer,
           base::UniquePtrComparator>  // for heterogeneous lookups
      background_web_contentses_;

  base::WeakPtrFactory<PlaylistBackgroundWebContentses> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTSES_H_
