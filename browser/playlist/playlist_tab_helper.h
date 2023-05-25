/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_PLAYLIST_TAB_HELPER_H_
#define BRAVE_BROWSER_PLAYLIST_PLAYLIST_TAB_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace playlist {

class PlaylistService;

class PlaylistTabHelper
    : public content::WebContentsUserData<PlaylistTabHelper>,
      public content::WebContentsObserver,
      public mojom::PlaylistServiceObserver {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents);

  ~PlaylistTabHelper() override;

  const std::vector<mojom::PlaylistItemPtr>& found_items() const {
    return found_items_;
  }

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DOMContentLoaded(content::RenderFrameHost* render_frame_host) override;

  // mojom::PlaylistServiceObserver:
  void OnEvent(mojom::PlaylistEvent event,
               const std::string& playlist_id) override {}
  void OnMediaFileDownloadProgressed(
      const std::string& id,
      int64_t total_bytes,
      int64_t received_bytes,
      int8_t percent_complete,
      const std::string& time_remaining) override {}
  void OnMediaFilesUpdated(const GURL& url,
                           std::vector<mojom::PlaylistItemPtr> items) override;

 private:
  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  // Hide factory function to enforce use MaybeCreateForWebContents()
  template <typename... Args>
  static void CreateForWebContents(content::WebContents*, Args&&...);

  PlaylistTabHelper(content::WebContents* contents, PlaylistService* service);

  void ResetData();
  void FindMediaFromCurrentContents();
  void OnFoundMediaFromContents(const GURL& url,
                                std::vector<mojom::PlaylistItemPtr> items);

  raw_ptr<PlaylistService> service_;

  std::vector<mojom::PlaylistItemPtr> found_items_;

  mojo::PendingReceiver<mojom::PlaylistServiceObserver>
      playlist_observer_receiver_;

  base::WeakPtrFactory<PlaylistTabHelper> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_PLAYLIST_PLAYLIST_TAB_HELPER_H_
