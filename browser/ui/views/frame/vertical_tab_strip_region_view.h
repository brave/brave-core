/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_

#include <memory>
#include <optional>

#include "base/functional/callback_helpers.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "base/types/pass_key.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_observer.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/resize_area_delegate.h"

namespace views {
class ResizeArea;
}

class BraveNewTabButton;
class BrowserView;
class FullscreenController;
class TabStripScrollContainer;
class VerticalTabStripScrollContentsView;

// Wraps TabStripRegion and show it vertically.
class VerticalTabStripRegionView : public views::View,
                                   public views::ResizeAreaDelegate,
                                   public views::AnimationDelegateViews,
                                   public views::WidgetObserver,
                                   public FullscreenObserver,
                                   public BrowserListObserver,
                                   public views::ContextMenuController {
  METADATA_HEADER(VerticalTabStripRegionView, views::View)
 public:
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
  State last_state() const { return last_state_; }
  bool is_animating() const { return width_animation_.is_animating(); }

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

  int GetTabStripViewportHeight() const;

  void set_layout_dirty(base::PassKey<VerticalTabStripScrollContentsView>) {
    layout_dirty_ = true;
  }

  void ResetExpandedWidth();
  bool IsMenuShowing() const;

  // views::View:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  gfx::Size GetMinimumSize() const override;
  void Layout(PassKey) override;
  void OnThemeChanged() override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void PreferredSizeChanged() override;
  void AddedToWidget() override;

  // views::ResizeAreaDelegate
  void OnResize(int resize_amount, bool done_resizing) override;

  // views::AnimationDelegateViews:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  // views::WidgetObserver:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;

  // FullscreenObserver:
  void OnFullscreenStateChanged() override;

  // views::ContextMenuController:
  void ShowContextMenuForViewImpl(
      views::View* source,
      const gfx::Point& p,
      ui::mojom::MenuSourceType source_type) override;

  class HeaderView;
  class MouseWatcher;

 private:
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, VisualState);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest,
                           OriginalTabSearchButton);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ExpandedState);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ExpandedWidth);

  FullscreenController* GetFullscreenController() const;
  bool IsTabFullscreen() const;
  bool IsBrowserFullscren() const;
  bool ShouldShowVerticalTabsInBrowserFullscreen() const;

  void SetState(State state);

  void SetExpandedWidth(int dest_width);

  void UpdateStateAfterDragAndDropFinished(State original_state);

  void OnShowVerticalTabsPrefChanged();
  void OnBrowserPanelsMoved();

  void UpdateLayout(bool in_destruction = false);

  void UpdateOriginalTabSearchButtonVisibility();

  void UpdateBorder();

  void OnCollapsedPrefChanged();
  void OnFloatingModePrefChanged();
  void OnExpandedStatePerWindowPrefChanged();
  void OnExpandedWidthPrefChanged();

  bool IsFloatingVerticalTabsEnabled() const;
  bool IsFloatingEnabledForBrowserFullscreen() const;
  void ScheduleFloatingModeTimer();
  void OnMouseExited();
  void OnMouseEntered();
  void OnMousePressedInTree();

  gfx::Size GetPreferredSizeForState(State state,
                                     bool include_border,
                                     bool ignore_animation) const;
  int GetPreferredWidthForState(State state,
                                bool include_border,
                                bool ignore_animation) const;

  // Returns valid object only when the related flag is enabled.
  TabStripScrollContainer* GetTabStripScrollContainer();

  std::u16string GetShortcutTextForNewTabButton(BrowserView* browser_view);

  void OnMenuClosed();

  views::LabelButton& GetToggleButtonForTesting();

  raw_ptr<BrowserView> browser_view_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;

  raw_ptr<views::View> original_parent_of_region_view_ = nullptr;
  raw_ptr<TabStripRegionView> original_region_view_ = nullptr;

  raw_ptr<HeaderView> header_view_ = nullptr;
  raw_ptr<views::View> contents_view_ = nullptr;

  // New tab button created for vertical tabs
  raw_ptr<BraveNewTabButton> new_tab_button_ = nullptr;

  raw_ptr<views::View> resize_area_ = nullptr;
  std::optional<int> resize_offset_;

  // A pointer storing the global tab style to be used.
  const raw_ptr<const TabStyle> tab_style_;

  State state_ = State::kExpanded;
  State last_state_ = State::kExpanded;

  BooleanPrefMember sidebar_side_;
  BooleanPrefMember show_vertical_tabs_;
  BooleanPrefMember collapsed_pref_;
  BooleanPrefMember expanded_state_per_window_pref_;
  BooleanPrefMember floating_mode_pref_;

  IntegerPrefMember expanded_width_pref_;
  int expanded_width_ = 220;

  base::OneShotTimer mouse_enter_timer_;

  bool mouse_events_for_test_ = false;

  bool layout_dirty_ = false;
  gfx::Size last_size_;

  gfx::SlideAnimation width_animation_{this};

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};

  std::unique_ptr<MouseWatcher> mouse_watcher_;

#if BUILDFLAG(IS_MAC)
  BooleanPrefMember show_toolbar_on_fullscreen_pref_;
#endif

  base::ScopedObservation<FullscreenController, FullscreenObserver>
      fullscreen_observation_{this};

  BooleanPrefMember vertical_tab_on_right_;

  std::unique_ptr<views::MenuRunner> menu_runner_;

  base::WeakPtrFactory<VerticalTabStripRegionView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_
