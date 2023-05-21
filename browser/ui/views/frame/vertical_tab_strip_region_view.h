/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_

#include <memory>

#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "base/types/pass_key.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "components/prefs/pref_member.h"
namespace views {
class ScrollView;
}

class BraveNewTabButton;
class BrowserView;
class TabStripScrollContainer;
class VerticalTabStripScrollContentsView;

// Wraps TabStripRegion and show it vertically.
class VerticalTabStripRegionView : public views::View,
                                   public TabStripModelObserver {
 public:
  METADATA_HEADER(VerticalTabStripRegionView);

  // We have a state machine which cycles like:
  //
  //               <hovered>          <pressed button>
  //   kCollapsed <----------> kFloating ----------> kExpanded
  //       ^        <exited>                            |
  //       |                                            |
  //       +--------------------------------------------+
  //                  <press button>
  //
  enum class State {
    kCollapsed,
    kFloating,
    kExpanded,
  };

  VerticalTabStripRegionView(BrowserView* browser_view,
                             TabStripRegionView* region_view);
  ~VerticalTabStripRegionView() override;

  State state() const { return state_; }

  const TabStrip* tab_strip() const {
    return original_region_view_->tab_strip_;
  }
  TabStrip* tab_strip() { return original_region_view_->tab_strip_; }

  const Browser* browser() const { return browser_; }

  // Expand vertical tabstrip temporarily. When the returned
  // ScopedCallbackRunner is destroyed, the state will be restored to the
  // previous state.
  using ScopedStateResetter = std::unique_ptr<base::ScopedClosureRunner>;
  [[nodiscard]] ScopedStateResetter ExpandTabStripForDragging();
  gfx::Vector2d GetOffsetForDraggedTab() const;

  int GetAvailableWidthForTabContainer();

  // This should be called when height of this view or tab strip changes.
  void UpdateNewTabButtonVisibility();

  TabSearchBubbleHost* GetTabSearchBubbleHost();

  int GetScrollViewViewportHeight() const;

  void set_layout_dirty(base::PassKey<VerticalTabStripScrollContentsView>) {
    layout_dirty_ = true;
  }

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  gfx::Size GetMinimumSize() const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void PreferredSizeChanged() override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

 private:
  class ScrollHeaderView;

  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, VisualState);

  bool IsTabFullscreen() const;

  void SetState(State state);

  void UpdateStateAfterDragAndDropFinished(State original_state);

  void OnShowVerticalTabsPrefChanged();

  void UpdateLayout(bool in_destruction = false);

  void UpdateTabSearchButtonVisibility();

  void OnCollapsedPrefChanged();
  void OnFloatingModePrefChanged();

  void ScheduleFloatingModeTimer();

  gfx::Size GetPreferredSizeForState(State state, bool include_border) const;
  int GetPreferredWidthForState(State state, bool include_border) const;

  // Returns valid object only when the related flag is enabled.
  TabStripScrollContainer* GetTabStripScrollContainer();

  void ScrollActiveTabToBeVisible();

  std::u16string GetShortcutTextForNewTabButton(BrowserView* browser_view);

  raw_ptr<Browser> browser_ = nullptr;

  raw_ptr<views::View> original_parent_of_region_view_ = nullptr;
  raw_ptr<TabStripRegionView> original_region_view_ = nullptr;

  // Contains TabStripRegion.
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> scroll_contents_view_ = nullptr;
  raw_ptr<ScrollHeaderView> scroll_view_header_ = nullptr;

  // New tab button created for vertical tabs
  raw_ptr<BraveNewTabButton> new_tab_button_ = nullptr;

  // A pointer storing the global tab style to be used.
  const raw_ptr<const TabStyle> tab_style_;

  State state_ = State::kExpanded;

  BooleanPrefMember show_vertical_tabs_;
  BooleanPrefMember collapsed_pref_;
  BooleanPrefMember floating_mode_pref_;

  base::OneShotTimer mouse_enter_timer_;

  bool mouse_events_for_test_ = false;

  bool layout_dirty_ = false;
  gfx::Size last_size_;

  base::WeakPtrFactory<VerticalTabStripRegionView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_
