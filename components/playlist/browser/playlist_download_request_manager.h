/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_

#include <vector>

#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"

namespace base {
class Value;
}  // namespace base

namespace playlist {

class PlaylistDownloadRequestManager {
 public:
  explicit PlaylistDownloadRequestManager(MediaDetectorComponentManager* manager);

  ~PlaylistDownloadRequestManager();

  MediaDetectorComponentManager* media_detector_component_manager() {
    return media_detector_component_manager_;
  }

  std::vector<mojom::PlaylistItemPtr> GetPlaylistItems(base::Value value,
                                                       GURL page_url);

  bool CanCacheMedia(const mojom::PlaylistItemPtr& item) const;
  bool ShouldExtractMediaFromBackgroundWebContents(
      const mojom::PlaylistItemPtr& item) const;

 private:
  raw_ptr<MediaDetectorComponentManager> media_detector_component_manager_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_
