/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTSES_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTSES_H_

#include <map>
#include <memory>

#include "base/containers/unique_ptr_adapters.h"
#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "url/gurl.h"

FORWARD_DECLARE_TEST(CosmeticFilteringPlaylistFlagEnabledTest,
                     AllowCosmeticFiltering);

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace playlist {

class PlaylistBackgroundWebContentses final {
 public:
  explicit PlaylistBackgroundWebContentses(content::BrowserContext* context);
  PlaylistBackgroundWebContentses(const PlaylistBackgroundWebContentses&) =
      delete;
  PlaylistBackgroundWebContentses& operator=(
      const PlaylistBackgroundWebContentses&) = delete;
  ~PlaylistBackgroundWebContentses();

  void Add(mojom::PlaylistItemPtr item,
           base::OnceCallback<void(mojom::PlaylistItemPtr)> callback,
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
              mojom::PlaylistItemPtr item,
              base::OnceCallback<void(mojom::PlaylistItemPtr)> callback,
              GURL url,
              bool is_media_source);

  // used by
  // PlaylistBackgroundWebContentsTest.ExtractPlaylistItemsInTheBackground, and
  // PlaylistBackgroundWebContentsTest.UserAgentOverride
  content::WebContents& web_contents() const {
    CHECK(background_web_contentses_.size() == 1);
    return *background_web_contentses_.cbegin()->first;
  }

  raw_ptr<content::BrowserContext> context_;
  std::map<std::unique_ptr<content::WebContents>,
           base::OneShotTimer,
           base::UniquePtrComparator>  // for heterogeneous lookups
      background_web_contentses_;

  base::WeakPtrFactory<PlaylistBackgroundWebContentses> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTSES_H_
