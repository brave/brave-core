/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_ICON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_ICON_VIEW_H_

#include <vector>

#include "base/callback_list.h"
#include "brave/browser/playlist/playlist_tab_helper.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"

class Browser;

class PlaylistActionIconView : public PageActionIconView {
 public:
  METADATA_HEADER(PlaylistActionIconView);

  PlaylistActionIconView(
      CommandUpdater* command_updater,
      Browser* browser,
      IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
      PageActionIconView::Delegate* page_action_icon_delegate);
  PlaylistActionIconView(const PlaylistActionIconView&) = delete;
  PlaylistActionIconView& operator=(const PlaylistActionIconView&) = delete;
  ~PlaylistActionIconView() override;

  views::BubbleDialogDelegate* GetBubble() const override;
  void OnExecuting(ExecuteSource execute_source) override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  void UpdateImpl() override;

 private:
  enum State { kNone, kAdded, kFound };

  playlist::PlaylistTabHelper* playlist_tab_helper() {
    return playlist::PlaylistTabHelper::FromWebContents(
        base::to_address(current_web_contents_));
  }

  void OnSavedItemChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& saved_items);
  void OnFoundItemChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& found_items);

  void UpdateState(int saved_items_count, int found_items_count);
  void UpdateVisibilityPerState();

  State state_ = kNone;

  raw_ptr<content::WebContents> current_web_contents_ = nullptr;

  base::CallbackListSubscription saved_items_changed_subscription_;
  base::CallbackListSubscription found_items_changed_subscription_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_ICON_VIEW_H_
