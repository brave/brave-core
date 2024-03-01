/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEBCONTENTS_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEBCONTENTS_H_

#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/playlist/browser/playlist_media_handler.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "third_party/abseil-cpp/absl/container/node_hash_map.h"

class GURL;

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace playlist {

class PlaylistService;

class PlaylistBackgroundWebContents final {
 public:
  PlaylistBackgroundWebContents(content::BrowserContext* context,
                                PlaylistService* service);
  PlaylistBackgroundWebContents(const PlaylistBackgroundWebContents&) = delete;
  PlaylistBackgroundWebContents& operator=(
      const PlaylistBackgroundWebContents&) = delete;
  ~PlaylistBackgroundWebContents();

  void Add(const GURL& url,
           PlaylistMediaHandler::OnceCallback on_media_detected_callback);

 private:
  void Remove(content::WebContents* web_contents,
              PlaylistMediaHandler::OnceCallback on_media_detected_callback,
              std::vector<mojom::PlaylistItemPtr> media,
              const GURL& url);

  raw_ptr<content::BrowserContext> context_;
  raw_ptr<PlaylistService> service_;
  absl::node_hash_map<std::unique_ptr<content::WebContents>, base::OneShotTimer>
      background_web_contents_;  // absl::node_hash_map<> supports
                                 // heterogeneous lookups

  base::WeakPtrFactory<PlaylistBackgroundWebContents> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEBCONTENTS_H_
