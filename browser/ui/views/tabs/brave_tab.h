// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_

#include <optional>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/tabs/accent_color/brave_tab_accent_types.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "ui/gfx/geometry/point.h"

namespace views {
class ImageButton;
}  // namespace views

namespace tabs {
class TreeTabNode;
}  // namespace tabs

namespace containers {
FORWARD_DECLARE_TEST(ContainersBrowserTest, SmallAccentIconViewVisibility);
}  // namespace containers

// Brave specific tab implementation that extends the base Tab class.
// It includes features like vertical tab support.
// Also customizes the tab layout and visual appearance for Brave's UI.
class BraveTab : public Tab {
  METADATA_HEADER(BraveTab, Tab)

 public:
  static constexpr int kTabAccentIconAreaWidth = 22;
  static constexpr int kExtraLeftPadding = 4;

  explicit BraveTab(tabs::TabHandle handle, TabSlotController* controller);
  BraveTab(const BraveTab&) = delete;
  BraveTab& operator=(const BraveTab&) = delete;
  ~BraveTab() override;

  // Resets tab_style_views_ so that it can have correct style for orientation.
  void UpdateTabStyle();

  // Updates the icon of the tree toggle button based on the collapsed state of
  // the tree tab node.
  void UpdateTreeToggleButtonIcon();

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
  void SetData(tabs::TabData data) override;
  bool IsActive() const override;
  TabSizeInfo GetTabSizeInfo() const override;
  TabNestingInfo GetTabNestingInfo() const override;
  bool IsInCollapsedTreeTabNode() const override;

  // Returns whether this tab should have an accent painted.
  bool ShouldPaintTabAccent() const;

  // Returns whether this tab should show a large accent icon on the left side.
  // Otherwise, it should show a small accent icon on the left-bottom cornder of
  // the tab.
  bool ShouldShowLargeAccentIcon() const;

  // Returns the accent colors (border, background) for this tab if it should
  // have an accent. Returns nullopt if the tab should not have an accent or
  // colors cannot be determined.
  std::optional<TabAccentColors> GetTabAccentColors() const;

  // Returns the accent icon for this tab if it should have an accent.
  // Returns an empty ImageModel if the tab should not have an accent or icon
  // cannot be determined.
  ui::ImageModel GetTabAccentIcon() const;

  base::WeakPtr<BraveTab> GetWeakPtr();

 private:
  friend class BraveTabTest;

  class SmallAccentIconView : public views::View {
    METADATA_HEADER(SmallAccentIconView, views::View)

   public:
    SmallAccentIconView();
    ~SmallAccentIconView() override;

    // views::View:
    void OnPaint(gfx::Canvas* canvas) override;
    gfx::Size CalculatePreferredSize(
        const views::SizeBounds& available_size) const override;
  };

  FRIEND_TEST_ALL_PREFIXES(BraveTabTest, ShouldAlwaysHideTabCloseButton);
  FRIEND_TEST_ALL_PREFIXES(BraveTabTest,
                           IconVisibilityWhenVerticalTabsAnimating);
  FRIEND_TEST_ALL_PREFIXES(
      BraveTabTest,
      PinnedTabIconCenteredWhenFloatingFromCompletelyHiddenMode);
  FRIEND_TEST_ALL_PREFIXES(BraveTabTestWithTreeTab,
                           TreeToggleButtonVisibleInsteadOfCloseButton);
  FRIEND_TEST_ALL_PREFIXES(
      BraveTabTestWithTreeTab,
      TreeToggleButtonAlwaysVisibleWhenCollapsedAndHasDescendants);
  FRIEND_TEST_ALL_PREFIXES(containers::ContainersBrowserTest,
                           SmallAccentIconViewVisibility);

  bool IsAtMinWidthForVerticalTabStrip() const;

  // Initializes the tree toggle button.
  void InitTreeToggleButton();

  // Called when the tree toggle button is pressed.
  void OnTreeToggleButtonPressed();

  // Returns whether this tab has tree tab node descendants.
  bool HasTreeTabNodeDescendants() const;

  // Lays out the tree toggle button. This will update the bounds and visibility
  // of the tree toggle button.
  void LayoutTreeToggleButton();

  // Returns whether the tree tab node is collapsed.
  bool IsTreeNodeCollapsed() const;

  raw_ptr<views::ImageButton> tree_toggle_button_ = nullptr;

  // Returns the tree tab node for this tab.
  const tabs::TreeTabNode* GetTreeTabNode() const;

  // Returns the height of the tree tab node for this tab.
  int GetTreeHeight() const;

  // Lays out the small tab accent icon view (visibility and bounds).
  void LayoutSmallTabAccentIcon();

  // Test accessors to reveal base class members.
  TabCloseButton* close_button_for_test() const { return close_button_.get(); }
  bool center_icon_for_test() const { return center_icon_; }
  bool showing_close_button_for_test() const { return showing_close_button_; }

  raw_ptr<SmallAccentIconView> small_accent_icon_view_ = nullptr;

  base::WeakPtrFactory<BraveTab> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
