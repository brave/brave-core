/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLES_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLES_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/views/view_tracker.h"

namespace views {
class View;
}  // namespace views

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

  void ShowBubble(views::View* anchor_view,
                  BubbleType bubble_type = BubbleType::kInfer);

  // Re-shows the bubble using the anchor view passed to the most recent
  // ShowBubble() call, if it's still alive. Used to chain from one bubble
  // type to another (e.g. "Add" to "Edit") across the async gap of a posted
  // task, when the original caller's own view may no longer be reachable.
  void ShowBubbleWithLastAnchor(BubbleType bubble_type);

  PlaylistBubbleView* GetBubble();

  void OnBubbleClosed();

  base::WeakPtr<PlaylistBubblesController> AsWeakPtr();

 private:
  friend class content::WebContentsUserData<PlaylistBubblesController>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  explicit PlaylistBubblesController(content::WebContents* web_contents);

  raw_ptr<PlaylistBubbleView> bubble_ = nullptr;
  views::ViewTracker last_anchor_view_tracker_;

  base::WeakPtrFactory<PlaylistBubblesController> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLES_CONTROLLER_H_
