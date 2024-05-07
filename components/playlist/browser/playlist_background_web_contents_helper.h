/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_

#include "base/functional/callback_forward.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace playlist {

class PlaylistBackgroundWebContentsHelper final
    : public content::WebContentsUserData<PlaylistBackgroundWebContentsHelper>,
      public content::WebContentsObserver {
 public:
  PlaylistBackgroundWebContentsHelper(
      const PlaylistBackgroundWebContentsHelper&) = delete;
  PlaylistBackgroundWebContentsHelper& operator=(
      const PlaylistBackgroundWebContentsHelper&) = delete;
  ~PlaylistBackgroundWebContentsHelper() override;

 private:
  friend class content::WebContentsUserData<
      PlaylistBackgroundWebContentsHelper>;

  PlaylistBackgroundWebContentsHelper(
      content::WebContents* web_contents,
      base::TimeDelta duration,
      base::OnceCallback<void(GURL, bool)> callback);

  // content::WebContentsObserver:
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;

  void GetMediaMetadata();

  base::TimeDelta duration_;
  base::OnceCallback<void(GURL, bool)> callback_;
  base::RepeatingTimer timer_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_
