/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEBCONTENTS_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEBCONTENTS_H_

#include <map>
#include <memory>
#include <vector>

#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/playlist/browser/playlist_media_handler.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace playlist {

class PlaylistService;

// `PlaylistBackgroundWebContents` fulfills background `WebContents` requests.
// After creating the background `WebContents`, it waits 10 seconds for the
// first non-empty media list to arrive. On receiving the media, or if the timer
// goes off (whichever happens first), it destructs the background
// `WebContents`, and calls the provided callback with the result.
// It overrides the user agent if `features::kPlaylistFakeUA` is enabled,
// or uses a static look-up table to decide if it has to otherwise.
class PlaylistBackgroundWebContents final {
 public:
  PlaylistBackgroundWebContents(content::BrowserContext* context,
                                PlaylistService* service);
  PlaylistBackgroundWebContents(const PlaylistBackgroundWebContents&) = delete;
  PlaylistBackgroundWebContents& operator=(
      const PlaylistBackgroundWebContents&) = delete;
  ~PlaylistBackgroundWebContents();

  void Add(const GURL& url,
           PlaylistMediaHandler::OnceCallback on_media_detected_callback,
           base::TimeDelta timeout = base::Seconds(10));

  void Reset();

 private:
  void Remove(content::WebContents* web_contents,
              PlaylistMediaHandler::OnceCallback on_media_detected_callback,
              GURL url,
              std::vector<mojom::PlaylistItemPtr> items);

  raw_ptr<content::BrowserContext> context_;
  raw_ptr<PlaylistService> service_;
  std::map<std::unique_ptr<content::WebContents>,
           base::OneShotTimer,
           base::UniquePtrComparator>  // for heterogeneous lookups
      background_web_contents_;

  base::WeakPtrFactory<PlaylistBackgroundWebContents> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEBCONTENTS_H_
