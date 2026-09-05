/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLES_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLES_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_user_data.h"

class PlaylistActionIconView;

namespace content {
class WebContents;
}

namespace playlist {
class PlaylistBubbleView;

class PlaylistBubblesController
    : public content::WebContentsUserData<PlaylistBubblesController> {
 public:
  enum class BubbleType { kInfer, kAdd, kEdit };

  static PlaylistBubblesController* CreateOrGetFromWebContents(
      content::WebContents* web_contents);

  ~PlaylistBubblesController() override;

  void ShowBubble(base::WeakPtr<PlaylistActionIconView> anchor_view,
                  BubbleType bubble_type = BubbleType::kInfer);

  PlaylistBubbleView* GetBubble();

  void OnBubbleClosed();

  base::WeakPtr<PlaylistBubblesController> AsWeakPtr();

 private:
  friend class content::WebContentsUserData<PlaylistBubblesController>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  explicit PlaylistBubblesController(content::WebContents* web_contents);

  raw_ptr<PlaylistBubbleView> bubble_ = nullptr;

  base::WeakPtrFactory<PlaylistBubblesController> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLES_CONTROLLER_H_
