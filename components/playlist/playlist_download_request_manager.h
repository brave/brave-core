/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_

#include <list>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/playlist/media_detector_component_manager.h"
#include "brave/components/playlist/playlist_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"

namespace base {
class OneShotTimer;
class Value;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace playlist {

struct CreatePlaylistParams;

// This class fetches each youtube playlist item's audio/video/thumbnail url and
// it's title by injecting media detector script to dedicated WebContents.
class PlaylistDownloadRequestManager
    : public MediaDetectorComponentManager::Observer,
      public content::WebContentsObserver {
 public:
  class Delegate {
   public:
    virtual void OnPlaylistCreationParamsReady(
        const CreatePlaylistParams& params) = 0;
  };

  static void SetPlaylistJavaScriptWorldId(const int32_t id);

  PlaylistDownloadRequestManager(content::BrowserContext* context,
                                 Delegate* delegate,
                                 MediaDetectorComponentManager* manager);
  ~PlaylistDownloadRequestManager() override;
  PlaylistDownloadRequestManager(const PlaylistDownloadRequestManager&) =
      delete;
  PlaylistDownloadRequestManager& operator=(
      const PlaylistDownloadRequestManager&) = delete;

  // Delegate will get called with generated param.
  void GeneratePlaylistCreateParamsForYoutubeURL(const std::string& url);

 private:
  // MediaDetectorComponentManager::Observer overrides:
  void OnScriptReady(const std::string& youtubedown_script) override;

  // content::WebContentsObserver overrides:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  // Will get data from youtube for downloading media files of |url| by
  // injecting media detector script
  void RunMediaDetector(const std::string& url);
  void OnGetMedia(base::Value value);
  bool ReadyToRunMediaDetectorScript() const;
  void CreateWebContents();
  void FetchAllPendingYoutubeURLs();

  void ScheduleWebContentsDestroying();
  void DestroyWebContents();

  // When we store youtube song, youtubedown js uses youtube song's url to fetch
  // it's metadata such as media file resource urls and thumbnail url.
  // and youtubedown js is injected to |web_contents_|.
  // We create |web_contents_| on demand. So, when youtube download is
  // requested, |web_contents_| may not be ready to inject youtubedown js. This
  // list caches already requested youtube song urls and used after
  // |web_contents_| is ready to use.
  std::list<std::string> pending_youtube_urls_;

  // Used to inject youtubedown.js to get playlist item metadata to download
  // its media files/thumbnail images and get titile.
  std::unique_ptr<content::WebContents> web_contents_;
  bool web_contents_ready_ = false;

  // The number of requested youtubedown data fetching.
  // If it's zero, all requested fetching are completed. Then |web_contents_|
  // destroying task will be scheduled.
  int in_progress_youtube_urls_count_ = 0;

  std::string media_detector_script_;
  raw_ptr<content::BrowserContext> context_;
  raw_ptr<Delegate> delegate_;

  raw_ptr<MediaDetectorComponentManager> media_detector_component_manager_;
  base::ScopedObservation<MediaDetectorComponentManager,
                          MediaDetectorComponentManager::Observer>
      observed_{this};
  std::unique_ptr<base::OneShotTimer> web_contents_destroy_timer_;

  base::WeakPtrFactory<PlaylistDownloadRequestManager> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_
