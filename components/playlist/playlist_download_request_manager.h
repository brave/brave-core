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
#include "base/scoped_observer.h"
#include "brave/components/playlist/playlist_types.h"
#include "brave/components/playlist/playlist_youtubedown_component_manager.h"
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
// it's title by injecting youtubedown.js script to dedicated WebContents.
class PlaylistDownloadRequestManager
    : public PlaylistYoutubeDownComponentManager::Observer,
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
                                 PlaylistYoutubeDownComponentManager* manager);
  ~PlaylistDownloadRequestManager() override;
  PlaylistDownloadRequestManager(const PlaylistDownloadRequestManager&) =
      delete;
  PlaylistDownloadRequestManager& operator=(
      const PlaylistDownloadRequestManager&) = delete;

  // Delegate will get called with generated param.
  void GeneratePlaylistCreateParamsForYoutubeURL(const std::string& url);

 private:
  // PlaylistYoutubeDownComponentManager::Observer overrides:
  void OnYoutubeDownScriptReady(const std::string& youtubedown_script) override;

  // content::WebContentsObserver overrides:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  // Will get data from youtube for downloading media files of |url| by
  // injecting youtubedown.js.
  void FetchYoutubeDownData(const std::string& url);
  void OnGetYoutubeDownData(base::Value value);
  bool ReadyToRunYoutubeDownJS() const;
  void CreateWebContents();
  void FetchAllPendingYoutubeURLs();

  void ScheduleWebContentsDestroying();
  void DestroyWebContents();

  // When we store youtube song, youtubedown js uses youtube song's url to fetch
  // it's metadata such as media file resource urls and thumbnail url.
  // and youtubedown js is injected to |webcontents_|.
  // We create |webcontents_| on demand. So, when youtube download is requested,
  // |webcontents_| may not be ready to inject youtubedown js.
  // This list caches already requested youtube song urls and used after
  // |webcontents_| is ready to use.
  std::list<std::string> pending_youtube_urls_;
  // Used to inject youtubedown.js to get playlist item metadata to download
  // its media files/thumbnail mages and get titile.
  std::unique_ptr<content::WebContents> webcontents_;
  bool webcontents_ready_ = false;
  // The number of requested youtubedown data fetching.
  // If it's zero, all requested fetching are completed. Then |webcontents_|
  // destroying task will be scheduled.
  int in_progress_youtube_urls_count_ = 0;
  std::string youtubedown_script_;
  content::BrowserContext* context_;
  Delegate* delegate_;
  PlaylistYoutubeDownComponentManager* youtubedown_component_manager_;
  ScopedObserver<PlaylistYoutubeDownComponentManager,
                 PlaylistYoutubeDownComponentManager::Observer>
      observed_{this};
  std::unique_ptr<base::OneShotTimer> webcontents_destroy_timer_;

  base::WeakPtrFactory<PlaylistDownloadRequestManager> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_
