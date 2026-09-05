/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_PLAYER_BRAVE_PLAYER_ACTION_ICON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_PLAYER_BRAVE_PLAYER_ACTION_ICON_VIEW_H_

#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "url/gurl.h"

class Browser;

class BravePlayerActionIconView : public PageActionIconView {
  METADATA_HEADER(BravePlayerActionIconView, PageActionIconView)
 public:

  BravePlayerActionIconView(
      CommandUpdater* command_updater,
      Browser& browser,
      IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
      PageActionIconView::Delegate* page_action_icon_delegate);
  BravePlayerActionIconView(const BravePlayerActionIconView&) = delete;
  BravePlayerActionIconView& operator=(const BravePlayerActionIconView&) =
      delete;
  ~BravePlayerActionIconView() override;

  // PageActionIconView:
  void OnExecuting(ExecuteSource execute_source) override;
  views::BubbleDialogDelegate* GetBubble() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  void UpdateIconImage() override;
  void UpdateImpl() override;
  void UpdateBorder() override;

 private:
  raw_ref<Browser> browser_;

  // A url to open brave player corresponding to the current tab.
  GURL player_url_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_PLAYER_BRAVE_PLAYER_ACTION_ICON_VIEW_H_
