/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"

namespace base {
class Value;
}  // namespace base

namespace blink::web_pref {
struct WebPreferences;
}  // namespace blink::web_pref

namespace content {
class BrowserContext;
}  // namespace content

namespace playlist {

// This class finds media files and their thumbnails and title from a page
// by injecting media detector script to dedicated WebContents.
class PlaylistDownloadRequestManager : public content::WebContentsObserver {
 public:
  struct Request {
    using Callback =
        base::OnceCallback<void(std::vector<mojom::PlaylistItemPtr>)>;

    Request();
    Request& operator=(const Request&) = delete;
    Request(const Request&) = delete;
    Request& operator=(Request&&) noexcept;
    Request(Request&&) noexcept;
    ~Request();

    absl::variant<std::string, base::WeakPtr<content::WebContents>>
        url_or_contents;

    bool should_force_fake_ua = false;

    Callback callback = base::NullCallback();
  };

  static void SetPlaylistJavaScriptWorldId(const int32_t id);

  PlaylistDownloadRequestManager(content::BrowserContext* context,
                                 MediaDetectorComponentManager* manager);
  ~PlaylistDownloadRequestManager() override;
  PlaylistDownloadRequestManager(const PlaylistDownloadRequestManager&) =
      delete;
  PlaylistDownloadRequestManager& operator=(
      const PlaylistDownloadRequestManager&) = delete;

  // Request::callback will be called with generated param.
  virtual void GetMediaFilesFromPage(Request request);

  // Update |web_prefs| if we want for |web_contents|.
  void ConfigureWebPrefsForBackgroundWebContents(
      content::WebContents* web_contents,
      blink::web_pref::WebPreferences* web_prefs);
  void ResetRequests();

  const content::WebContents* background_contents() const {
    return web_contents_.get();
  }

  // This will create or get web contents
  content::WebContents* GetBackgroundWebContentsForTesting();

  const MediaDetectorComponentManager* media_detector_component_manager()
      const {
    return media_detector_component_manager_;
  }
  MediaDetectorComponentManager* media_detector_component_manager() {
    return media_detector_component_manager_;
  }

  void SetRunScriptOnMainWorldForTest();

 private:
  // Calling this will trigger loading |url| on a web contents,
  // and we'll inject javascript on the contents to get a list of
  // media files on the page.
  void RunMediaDetector(Request request);

  bool ReadyToRunMediaDetectorScript() const;
  void CreateWebContents(bool should_force_fake_ua);
  void GetMedia(content::WebContents* contents);
  void OnGetMedia(base::WeakPtr<content::WebContents> contents,
                  base::Value value);
  void ProcessFoundMedia(base::WeakPtr<content::WebContents> contents,
                         base::Value value);

  // Pop a task from queue and detect media from the page if any.
  void FetchPendingRequest();

  // content::WebContentsObserver overrides:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  // We create |web_contents_| on demand. So, when downloading media is
  // requested, |web_contents_| may not be ready to inject js script. This
  // list caches already requested urls and used after |web_contents_| is
  // ready to use.
  std::list<Request> pending_requests_;

  // Used to inject js script to get playlist item metadata to download
  // its media files/thumbnail images and get title.
  std::unique_ptr<content::WebContents> web_contents_;

  // The number of requested data fetching.
  // If it's zero, all requested fetching are completed. Then |web_contents_|
  // destroying task will be scheduled.
  int in_progress_urls_count_ = 0;
  GURL requested_url_;
  Request::Callback callback_for_current_request_ = base::NullCallback();
  base::Time request_start_time_;

  raw_ptr<content::BrowserContext> context_;

  raw_ptr<MediaDetectorComponentManager> media_detector_component_manager_;

  bool run_script_on_main_world_ = false;

  base::WeakPtrFactory<PlaylistDownloadRequestManager> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_DOWNLOAD_REQUEST_MANAGER_H_
