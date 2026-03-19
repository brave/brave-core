/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/types/pass_key.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/view_shadow.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveActionsContainer;
class BraveActionsContainerTest;
class BraveShieldsPageInfoController;
class PromotionButtonController;
class PromotionButtonView;
class PlaylistActionIconView;
class RewardsBrowserTest;
class SkPath;

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
class BraveNewsActionIconView;
#endif

#if BUILDFLAG(ENABLE_TOR)
class OnionLocationView;
#endif

namespace playlist {
FORWARD_DECLARE_TEST(PlaylistBrowserTest, AddItemsToList);
FORWARD_DECLARE_TEST(PlaylistBrowserTest, UIHiddenWhenDisabled);
FORWARD_DECLARE_TEST(PlaylistBrowserTestWithSitesUsingMediaSource,
                     MediaShouldBeExtractedFromBackground_SucceedInExtracting);
FORWARD_DECLARE_TEST(PlaylistBrowserTestWithSitesUsingMediaSource,
                     MediaShouldBeExtractedFromBackground_FailToExtract);
FORWARD_DECLARE_TEST(
    PlaylistBrowserTestWithSitesUsingMediaSource,
    MediaShouldBeExtractedFromBackground_DynamicallyAddedMedia);
}  // namespace playlist

namespace policy {
FORWARD_DECLARE_TEST(BraveRewardsPolicyTest, RewardsIconIsHidden);
}

// The purposes of this subclass are to:
// - Add the BraveActionsContainer to the location bar
class BraveLocationBarView : public LocationBarView {
  METADATA_HEADER(BraveLocationBarView, LocationBarView)
 public:
  BraveLocationBarView(Browser* browser,
                       Profile* profile,
                       CommandUpdater* command_updater,
                       Delegate* delegate,
                       bool is_popup_mode);
  ~BraveLocationBarView() override;

  BraveLocationBarView(const BraveLocationBarView&) = delete;
  BraveLocationBarView& operator=(const BraveLocationBarView&) = delete;

  void Init() override;
  void Update(content::WebContents* contents) override;
  void OnChanged() override;
  BraveActionsContainer* GetBraveActionsContainer() { return brave_actions_; }
#if BUILDFLAG(ENABLE_TOR)
  OnionLocationView* GetOnionLocationView() { return onion_location_view_; }
#endif

  // LocationBarView:
  // Views that locates at right side of upstream's trailing views.
  std::vector<views::View*> GetRightMostTrailingViews() override;
  // Views that locates at left side of upstream's trailing views.
  std::vector<views::View*> GetLeftMostTrailingViews() override;
  views::View* GetSearchPromotionButton() const override;
  void RefreshBackground() override;
  void OnOmniboxBlurred() override;
  void Layout(PassKey) override;
  void OnVisibleBoundsChanged() override;

  // views::View:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void ChildVisibilityChanged(views::View* child) override;
  void AddedToWidget() override;
  int GetBorderRadius() const override;
  void FocusLocation(bool is_user_initiated) override;

  SkPath GetFocusRingHighlightPath() const;
  ContentSettingImageView* GetContentSettingsImageViewForTesting(size_t idx);
  BraveActionsContainer* brave_actions_contatiner_view() {
    return brave_actions_;
  }

  void ShowPlaylistBubble(
      playlist::PlaylistBubblesController::BubbleType type =
          playlist::PlaylistBubblesController::BubbleType::kInfer);

  void set_ignore_layout(base::PassKey<BraveToolbarView::LayoutGuard>,
                         bool ignore) {
    ignore_layout_ = ignore;
  }

 private:
  FRIEND_TEST_ALL_PREFIXES(playlist::PlaylistBrowserTest, AddItemsToList);
  FRIEND_TEST_ALL_PREFIXES(playlist::PlaylistBrowserTest, UIHiddenWhenDisabled);
  FRIEND_TEST_ALL_PREFIXES(
      playlist::PlaylistBrowserTestWithSitesUsingMediaSource,
      MediaShouldBeExtractedFromBackground_SucceedInExtracting);
  FRIEND_TEST_ALL_PREFIXES(
      playlist::PlaylistBrowserTestWithSitesUsingMediaSource,
      MediaShouldBeExtractedFromBackground_FailToExtract);
  FRIEND_TEST_ALL_PREFIXES(
      playlist::PlaylistBrowserTestWithSitesUsingMediaSource,
      MediaShouldBeExtractedFromBackground_DynamicallyAddedMedia);
  FRIEND_TEST_ALL_PREFIXES(policy::BraveRewardsPolicyTest, RewardsIconIsHidden);
  FRIEND_TEST_ALL_PREFIXES(BraveLocationBarViewBrowserTest,
                           SearchConversionButtonTest);
  friend class ::BraveActionsContainerTest;
  friend class ::RewardsBrowserTest;

  PlaylistActionIconView* GetPlaylistActionIconView();
  void SetupShadow();

  // Hides/shows Brave trailing views based on available bar width so that the
  // omnibox has priority over Brave-specific buttons when space is tight.
  // Must be called before LayoutSuperclass<LocationBarView>() in Layout().
  void UpdateBraveViewsSpaceVisibility();

  // Prevent layout with invalid rect.
  // It also could make omnibox popup have wrong position.
  // See the comments of BraveToolbarView::Layout().
  bool ignore_layout_ = false;

  // True when Brave trailing views were hidden because there was not enough
  // space for both them and the omnibox minimum width. Used to restore their
  // natural visibility when the bar becomes wider.
  bool brave_views_collapsed_for_space_ = false;

  // True when upstream trailing views (translate, share, etc.) were hidden
  // because brave_actions_ and the omnibox minimum claimed all available space.
  // Tracked separately from brave_views_collapsed_for_space_ so that each set
  // of views is restored independently.
  bool upstream_trailing_collapsed_for_space_ = false;
  // Pre-collapse visibility states for upstream trailing views, so that we only
  // restore views that were actually visible before we hid them.
  bool page_action_icon_container_was_visible_ = false;
  bool page_action_container_was_visible_ = false;

  // Guards against re-entrant layout calls that can occur when SetVisible()
  // is called on a Brave trailing view inside UpdateBraveViewsSpaceVisibility()
  // (which would otherwise trigger ChildVisibilityChanged() →
  // DeprecatedLayoutImmediately() → Layout() recursion).
  bool in_brave_space_update_ = false;
  std::unique_ptr<ViewShadow> shadow_;
  raw_ptr<BraveActionsContainer> brave_actions_ = nullptr;
#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  raw_ptr<BraveNewsActionIconView> brave_news_action_icon_view_ = nullptr;
#endif
  std::unique_ptr<PromotionButtonController> promotion_controller_;
  raw_ptr<PromotionButtonView> promotion_button_ = nullptr;
  std::unique_ptr<BraveShieldsPageInfoController> shields_page_info_controller_;
#if BUILDFLAG(ENABLE_TOR)
  raw_ptr<OnionLocationView> onion_location_view_ = nullptr;
#endif
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_
