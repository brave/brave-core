/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_ACTION_ICON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_ACTION_ICON_VIEW_H_

#include "base/memory/raw_ref.h"
#include "brave/browser/ui/views/page_action/wayback_machine_state_manager.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"

class Browser;

// Shows current page's wayback state. It's only shown when loaded page
// is missing. Icon gets current page's wayback state from
// WaybackMachineStateManager.
class WaybackMachineActionIconView : public PageActionIconView {
  METADATA_HEADER(WaybackMachineActionIconView, PageActionIconView)
 public:
  WaybackMachineActionIconView(
      CommandUpdater* command_updater,
      Browser* browser,
      IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
      PageActionIconView::Delegate* page_action_icon_delegate);

  ~WaybackMachineActionIconView() override;

  // PageActionIconView overrides:
  views::BubbleDialogDelegate* GetBubble() const override;
  void OnExecuting(ExecuteSource source) override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  ui::ImageModel GetSizedIconImage(int size) const override;
  void UpdateImpl() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWaybackMachineTest, BubbleLaunchTest);

  void ExecuteCommandForTesting();

  WaybackMachineStateManager state_manager_;
  raw_ref<Browser> browser_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_ACTION_ICON_VIEW_H_
