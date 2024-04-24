/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLES_ICON_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLES_ICON_CONTROLLER_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

class PlaylistActionIconView;

namespace gfx {
struct VectorIcon;
}  // namespace gfx

namespace playlist {

class PlaylistActionBubbleView;
class PlaylistTabHelper;

class PlaylistBubblesController
    : public content::WebContentsUserData<PlaylistBubblesController> {
 public:
  static PlaylistBubblesController* CreateOrGetFromWebContents(
      content::WebContents* web_contents);

  ~PlaylistBubblesController() override;

  void ShowBubble(base::WeakPtr<PlaylistActionIconView> anchor_view);
  void ShowBubble(std::unique_ptr<PlaylistActionBubbleView> bubble);
  PlaylistActionBubbleView* GetBubble() const;

  base::WeakPtr<PlaylistBubblesController> AsWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  void OnBubbleClosed();

 protected:
  explicit PlaylistBubblesController(content::WebContents* web_contents);

 private:
  friend class content::WebContentsUserData<PlaylistBubblesController>;

  raw_ptr<PlaylistActionBubbleView> bubble_ = nullptr;

  base::WeakPtrFactory<PlaylistBubblesController> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLES_ICON_CONTROLLER_H_
