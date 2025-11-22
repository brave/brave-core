// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_

#include <optional>
#include <string>

#include "base/gtest_prod_util.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/mouse_watcher.h"

namespace tabs {
class TreeTabNode;
}  // namespace tabs

// Brave specific tab implementation that extends the base Tab class.
// It includes features like vertical tab support, renaming functionality.
// Also customizes the tab layout and visual appearance for Brave's UI.
class BraveTab : public Tab, public views::TextfieldController {
 public:
  explicit BraveTab(TabSlotController* controller);
  BraveTab(const BraveTab&) = delete;
  BraveTab& operator=(const BraveTab&) = delete;
  ~BraveTab() override;

  void EnterRenameMode();

  // Returns the tabs::TreeTabNode that this tab is associated with.
  const tabs::TreeTabNode* GetTreeTabNode() const;

  // Returns the height of the tree that this tab belongs to.
  int GetTreeHeight() const;

  // Tab:
  std::u16string GetRenderedTooltipText(const gfx::Point& p) const override;

  // Overridden because we moved alert button to left side in the tab whereas
  // upstream put it on right side. Need to consider this change for calculating
  // largest selectable region.
  int GetWidthOfLargestSelectableRegion() const override;

  void ActiveStateChanged() override;

  std::optional<SkColor> GetGroupColor() const override;

  void UpdateIconVisibility() override;
  bool ShouldRenderAsNormalTab() const override;
  void Layout(PassKey) override;
  void MaybeAdjustLeftForPinnedTab(gfx::Rect* bounds,
                                   int visual_width) const override;
  gfx::Insets GetInsets() const override;
  void SetData(TabRendererData data) override;
  bool IsActive() const override;
  TabNestingInfo GetTabNestingInfo() const override;

  // views::TextfieldController:
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

 private:
  friend class BraveTabTest;
  friend class BraveTabRenamingUnitTest;

  FRIEND_TEST_ALL_PREFIXES(BraveTabTest, ShouldAlwaysHideTabCloseButton);

  FRIEND_TEST_ALL_PREFIXES(BraveTabRenamingUnitTest,
                           ClickingOutsideRenamingTabCommitsRename);

  // A textfield used for renaming tabs. It is a child of BraveTab and
  // automatically handles mouse clicks outside of it to exit rename mode.
  class RenameTextfield : public views::Textfield,
                          public views::MouseWatcherListener {
    METADATA_HEADER(RenameTextfield, views::Textfield)
   public:
    explicit RenameTextfield(base::RepeatingClosure on_click_outside_callback);
    ~RenameTextfield() override;

    // views::Textfield:
    void VisibilityChanged(views::View* starting_from,
                           bool is_visible) override;

    // views::MouseWatcherListener:
    void MouseMovedOutOfHost() override;

   private:
    // Callback to be invoked when mouse is clicked outside of the textfield.
    base::RepeatingClosure on_click_outside_callback_;

    // Mouse watcher that tracks mouse clicks outside of the textfield.
    views::MouseWatcher mouse_watcher_;
  };

  bool IsAtMinWidthForVerticalTabStrip() const;

  void CommitRename();
  void ExitRenameMode();
  void UpdateRenameTextfieldBounds();

  bool in_renaming_mode() const {
    return rename_textfield_ && rename_textfield_->GetVisible();
  }

  // Reveals the title label which is in base class member.
  views::Label* title_for_test() const { return title_; }

  // Reveals the close button which is in base class member.
  TabCloseButton* close_button_for_test() const { return close_button_.get(); }

  raw_ptr<RenameTextfield> rename_textfield_ = nullptr;

  static constexpr int kExtraLeftPadding = 4;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
